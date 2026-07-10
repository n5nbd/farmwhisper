#include "fw_ble.h"

#include "fw_device_config.h"
#include "fw_radio_profile.h"
#include "fw_transport_mode.h"

#include <BLEAdvertising.h>
#include <BLECharacteristic.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEService.h>
#include <esp_mac.h>

#include <cstring>

namespace {

constexpr size_t kDeviceIdLength = 10;
constexpr size_t kMaxLegacyLocalNameLength = 26;
constexpr uint32_t kAdvertisingRestartDelayMs = 100;

constexpr const char *kDeviceInformationServiceUuid = "180A";
constexpr const char *kManufacturerNameCharacteristicUuid = "2A29";
constexpr const char *kModelNumberCharacteristicUuid = "2A24";
constexpr const char *kSerialNumberCharacteristicUuid = "2A25";
constexpr const char *kFirmwareRevisionCharacteristicUuid = "2A26";
constexpr const char *kHardwareRevisionCharacteristicUuid = "2A27";

constexpr const char *kConfigurationServiceUuid =
    "3f4b0001-7d5a-4b22-9e2f-5c7a9f6d1000";
constexpr const char *kDeviceAliasCharacteristicUuid =
    "3f4b0002-7d5a-4b22-9e2f-5c7a9f6d1000";
constexpr const char *kRadioProfileCharacteristicUuid =
    "3f4b0003-7d5a-4b22-9e2f-5c7a9f6d1000";
constexpr const char *kTransportModeCharacteristicUuid =
    "3f4b0004-7d5a-4b22-9e2f-5c7a9f6d1000";

constexpr const char *kManufacturerName = "FarmWhisper";
constexpr const char *kModelNumber = "FW100";
constexpr const char *kFirmwareRevision = "development";
constexpr const char *kHardwareRevision = "Heltec V4 R2/R8";

bool bleStackInitialized = false;
bool deviceInformationReady = false;
bool configurationServiceReady = false;
bool advertisingActive = false;
bool clientConnected = false;
bool policyReported = false;
uint32_t advertisingRestartAfterMs = 0;

BLEServer *bleServer = nullptr;
BLEService *deviceInformationService = nullptr;
BLEService *configurationService = nullptr;
BLECharacteristic *deviceAliasCharacteristic = nullptr;
BLECharacteristic *radioProfileCharacteristic = nullptr;
BLECharacteristic *transportModeCharacteristic = nullptr;
Stream *diagnosticOut = nullptr;

char deviceId[kDeviceIdLength + 1] = {};
char activeAdvertisingName[kMaxLegacyLocalNameLength + 1] = {};

enum class ConfigurationField {
  DeviceAlias,
  RadioProfile,
  TransportMode,
};

void refreshConfigurationValues();

const char *configurationFieldName(ConfigurationField field) {
  switch (field) {
    case ConfigurationField::DeviceAlias:
      return "device-alias";
    case ConfigurationField::RadioProfile:
      return "radio-profile";
    case ConfigurationField::TransportMode:
      return "transport-mode";
  }

  return "unknown";
}

const char *configurationFieldValue(ConfigurationField field) {
  switch (field) {
    case ConfigurationField::DeviceAlias:
      return fwDeviceAlias();
    case ConfigurationField::RadioProfile:
      return fwRadioProfileKey(fwSelectedRadioProfileId());
    case ConfigurationField::TransportMode:
      return fwTransportModeKey(fwSelectedTransportModeId());
  }

  return "";
}

void logConfigurationWrite(ConfigurationField field,
                           bool accepted,
                           const String &normalizedValue) {
  if (diagnosticOut == nullptr) {
    return;
  }

  diagnosticOut->printf("[ble] configuration write field=%s result=%s value=\"%s\"\n",
                        configurationFieldName(field),
                        accepted ? "accepted" : "rejected",
                        normalizedValue.c_str());
}

bool applyConfigurationWrite(ConfigurationField field,
                             const char *rawValue,
                             size_t rawValueLength,
                             String &normalizedValue) {
  if (rawValue == nullptr || rawValueLength != strlen(rawValue)) {
    normalizedValue = "";
    return false;
  }

  normalizedValue = rawValue;
  normalizedValue.trim();

  switch (field) {
    case ConfigurationField::DeviceAlias:
      if (normalizedValue.length() == 0) {
        fwClearDeviceAlias();
        return true;
      }
      return fwSetDeviceAlias(normalizedValue.c_str());

    case ConfigurationField::RadioProfile:
      return fwSetSelectedRadioProfileByKey(normalizedValue.c_str());

    case ConfigurationField::TransportMode:
      return fwSetSelectedTransportModeByKey(normalizedValue.c_str());
  }

  return false;
}

class ConfigurationCharacteristicCallbacks : public BLECharacteristicCallbacks {
 public:
  explicit ConfigurationCharacteristicCallbacks(ConfigurationField field)
      : field_(field) {}

  void onRead(BLECharacteristic *characteristic) override {
    if (characteristic == nullptr) {
      return;
    }

    const char *value = configurationFieldValue(field_);
    characteristic->setValue(
        reinterpret_cast<uint8_t *>(const_cast<char *>(value)), strlen(value));
  }

  void onWrite(BLECharacteristic *characteristic) override {
    if (characteristic == nullptr) {
      return;
    }

    const auto rawValue = characteristic->getValue();
    String normalizedValue;
    const bool accepted = applyConfigurationWrite(
        field_, rawValue.c_str(), rawValue.length(), normalizedValue);

    refreshConfigurationValues();
    logConfigurationWrite(field_, accepted, normalizedValue);
  }

 private:
  ConfigurationField field_;
};

ConfigurationCharacteristicCallbacks deviceAliasCallbacks(
    ConfigurationField::DeviceAlias);
ConfigurationCharacteristicCallbacks radioProfileCallbacks(
    ConfigurationField::RadioProfile);
ConfigurationCharacteristicCallbacks transportModeCallbacks(
    ConfigurationField::TransportMode);

class FarmWhisperServerCallbacks : public BLEServerCallbacks {
 public:
  void onConnect(BLEServer *) override {
    clientConnected = true;
    advertisingActive = false;
    advertisingRestartAfterMs = 0;
    activeAdvertisingName[0] = '\0';

    if (diagnosticOut != nullptr) {
      diagnosticOut->println("[ble] client connected");
    }
  }

  void onDisconnect(BLEServer *) override {
    clientConnected = false;
    advertisingActive = false;
    activeAdvertisingName[0] = '\0';
    advertisingRestartAfterMs = millis() + kAdvertisingRestartDelayMs;

    if (diagnosticOut != nullptr) {
      diagnosticOut->println("[ble] client disconnected; advertising restart scheduled");
    }
  }
};

FarmWhisperServerCallbacks serverCallbacks;

void buildDeviceId() {
  uint8_t mac[6] = {};
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  snprintf(deviceId,
           sizeof(deviceId),
           "FWP-%02X%02X%02X",
           mac[3],
           mac[4],
           mac[5]);
}

void buildAdvertisingName(char *destination, size_t destinationSize) {
  if (destination == nullptr || destinationSize == 0) {
    return;
  }

  const char *alias = fwDeviceAlias();
  if (alias == nullptr || alias[0] == '\0') {
    snprintf(destination, destinationSize, "%s", deviceId);
    return;
  }

  char safeAlias[kFwDeviceAliasMaxLen + 1] = {};
  size_t safeAliasLength = 0;
  for (size_t index = 0;
       alias[index] != '\0' && safeAliasLength < kFwDeviceAliasMaxLen;
       ++index) {
    const uint8_t character = static_cast<uint8_t>(alias[index]);
    safeAlias[safeAliasLength++] =
        character >= 0x20 && character <= 0x7E
            ? static_cast<char>(character)
            : '?';
  }
  safeAlias[safeAliasLength] = '\0';

  const size_t prefixLength = strlen(deviceId) + 1;
  const size_t maximumAliasLength =
      kMaxLegacyLocalNameLength > prefixLength
          ? kMaxLegacyLocalNameLength - prefixLength
          : 0;

  snprintf(destination,
           destinationSize,
           "%s %.*s",
           deviceId,
           static_cast<int>(maximumAliasLength),
           safeAlias);
}

bool restartDelayElapsed() {
  if (advertisingRestartAfterMs == 0) {
    return true;
  }

  return static_cast<int32_t>(millis() - advertisingRestartAfterMs) >= 0;
}

void setReadOnlyTextCharacteristic(BLEService *service,
                                   const char *uuid,
                                   const char *value) {
  BLECharacteristic *characteristic = service->createCharacteristic(
      uuid, BLECharacteristic::PROPERTY_READ);
  characteristic->setValue(
      reinterpret_cast<uint8_t *>(const_cast<char *>(value)), strlen(value));
}

void setCharacteristicValue(BLECharacteristic *characteristic,
                            const char *value) {
  if (characteristic == nullptr) {
    return;
  }

  const char *desiredValue = value == nullptr ? "" : value;
  const size_t desiredLength = strlen(desiredValue);
  const auto currentValue = characteristic->getValue();

  if (currentValue.length() != desiredLength ||
      memcmp(currentValue.c_str(), desiredValue, desiredLength) != 0) {
    characteristic->setValue(
        reinterpret_cast<uint8_t *>(const_cast<char *>(desiredValue)),
        desiredLength);
  }
}

void refreshConfigurationValues() {
  if (!configurationServiceReady) {
    return;
  }

  setCharacteristicValue(deviceAliasCharacteristic, fwDeviceAlias());
  setCharacteristicValue(
      radioProfileCharacteristic,
      fwRadioProfileKey(fwSelectedRadioProfileId()));
  setCharacteristicValue(
      transportModeCharacteristic,
      fwTransportModeKey(fwSelectedTransportModeId()));
}

void ensureDeviceInformationService(Stream &out) {
  if (deviceInformationReady) {
    return;
  }

  if (bleServer == nullptr) {
    bleServer = BLEDevice::createServer();
    bleServer->setCallbacks(&serverCallbacks);
  }

  deviceInformationService =
      bleServer->createService(kDeviceInformationServiceUuid);

  setReadOnlyTextCharacteristic(deviceInformationService,
                                kManufacturerNameCharacteristicUuid,
                                kManufacturerName);
  setReadOnlyTextCharacteristic(deviceInformationService,
                                kModelNumberCharacteristicUuid,
                                kModelNumber);
  setReadOnlyTextCharacteristic(deviceInformationService,
                                kSerialNumberCharacteristicUuid,
                                deviceId);
  setReadOnlyTextCharacteristic(deviceInformationService,
                                kFirmwareRevisionCharacteristicUuid,
                                kFirmwareRevision);
  setReadOnlyTextCharacteristic(deviceInformationService,
                                kHardwareRevisionCharacteristicUuid,
                                kHardwareRevision);

  deviceInformationService->start();
  deviceInformationReady = true;

  out.printf("[ble] Device Information Service ready uuid=%s readOnly=yes\n",
             kDeviceInformationServiceUuid);
}

void ensureConfigurationService(Stream &out) {
  if (configurationServiceReady) {
    return;
  }

  if (bleServer == nullptr) {
    bleServer = BLEDevice::createServer();
    bleServer->setCallbacks(&serverCallbacks);
  }

  configurationService = bleServer->createService(kConfigurationServiceUuid);

  deviceAliasCharacteristic = configurationService->createCharacteristic(
      kDeviceAliasCharacteristicUuid,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  deviceAliasCharacteristic->setCallbacks(&deviceAliasCallbacks);

  radioProfileCharacteristic = configurationService->createCharacteristic(
      kRadioProfileCharacteristicUuid,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  radioProfileCharacteristic->setCallbacks(&radioProfileCallbacks);

  transportModeCharacteristic = configurationService->createCharacteristic(
      kTransportModeCharacteristicUuid,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  transportModeCharacteristic->setCallbacks(&transportModeCallbacks);

  configurationServiceReady = true;
  refreshConfigurationValues();
  configurationService->start();

  out.printf(
      "[ble] Configuration Service ready uuid=%s properties=read,write "
      "notifications=no\n",
      kConfigurationServiceUuid);
}

void stopAdvertising(Stream &out, const char *reason) {
  if (!advertisingActive) {
    return;
  }

  BLEDevice::getAdvertising()->stop();
  advertisingActive = false;
  activeAdvertisingName[0] = '\0';

  out.printf("[ble] advertising stopped reason=%s\n", reason);
}

void startAdvertising(Stream &out, const char *advertisingName) {
  if (!bleStackInitialized) {
    BLEDevice::init(advertisingName);
    bleStackInitialized = true;
  }

  ensureDeviceInformationService(out);
  ensureConfigurationService(out);

  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  advertising->stop();

  BLEAdvertisementData advertisementData;
  advertisementData.setFlags(0x06);
  advertisementData.setName(advertisingName);
  advertisementData.setCompleteServices(
      BLEUUID(kDeviceInformationServiceUuid));

  advertising->setAdvertisementData(advertisementData);
  advertising->setScanResponse(false);
  advertising->setAdvertisementType(ADV_TYPE_IND);
  advertising->start();

  snprintf(activeAdvertisingName,
           sizeof(activeAdvertisingName),
           "%s",
           advertisingName);
  advertisingActive = true;
  advertisingRestartAfterMs = 0;

  out.printf(
      "[ble] advertising started name=\"%s\" advertisedService=%s "
      "configurationService=%s\n",
      activeAdvertisingName,
      kDeviceInformationServiceUuid,
      kConfigurationServiceUuid);
}

void reconcile(Stream &out, bool forcePolicyReport) {
  const FwTransportModeId selectedMode = fwSelectedTransportModeId();
  const bool shouldAdvertise = fwTransportModeUsesBluetoothLe(selectedMode);

  if (forcePolicyReport || !policyReported) {
    out.printf("[ble] transport=%s enabled=%s\n",
               fwTransportModeKey(selectedMode),
               shouldAdvertise ? "yes" : "no");
    policyReported = true;
  }

  if (!shouldAdvertise) {
    advertisingRestartAfterMs = 0;
    stopAdvertising(out, "transport-disabled");
    return;
  }

  if (clientConnected) {
    return;
  }

  char desiredAdvertisingName[kMaxLegacyLocalNameLength + 1] = {};
  buildAdvertisingName(desiredAdvertisingName,
                       sizeof(desiredAdvertisingName));

  if (advertisingActive &&
      strcmp(activeAdvertisingName, desiredAdvertisingName) == 0) {
    return;
  }

  if (advertisingActive) {
    stopAdvertising(out, "identity-changed");
    advertisingRestartAfterMs = millis() + kAdvertisingRestartDelayMs;
  }

  if (!restartDelayElapsed()) {
    return;
  }

  startAdvertising(out, desiredAdvertisingName);
}

}  // namespace

namespace FWBLE {

void begin(Stream &out) {
  diagnosticOut = &out;
  buildDeviceId();
  reconcile(out, true);
}

void service(Stream &out) {
  diagnosticOut = &out;
  reconcile(out, false);
}

bool isAdvertising() {
  return advertisingActive;
}

const char *advertisedName() {
  return activeAdvertisingName;
}

}  // namespace FWBLE
