#include "fw_ble.h"

#include "fw_device_config.h"
#include "fw_transport_mode.h"

#include <BLEAdvertising.h>
#include <BLECharacteristic.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEService.h>
#include <BLEUUID.h>
#include <esp_mac.h>

#include <cstring>
#include <string>

namespace {

constexpr size_t kDeviceIdLength = 10; // "FWP-" + six hex digits
constexpr size_t kMaxLegacyLocalNameLength = 26;
constexpr uint32_t kAdvertisingRestartDelayMs = 100;

constexpr uint16_t kDeviceInformationServiceUuid = 0x180A;
constexpr uint16_t kManufacturerNameUuid = 0x2A29;
constexpr uint16_t kModelNumberUuid = 0x2A24;
constexpr uint16_t kSerialNumberUuid = 0x2A25;
constexpr uint16_t kFirmwareRevisionUuid = 0x2A26;
constexpr uint16_t kHardwareRevisionUuid = 0x2A27;

constexpr const char *kManufacturerName = "FarmWhisper";
constexpr const char *kModelNumber = "FW100";
constexpr const char *kFirmwareRevision = "development";
constexpr const char *kHardwareRevision = "Heltec V4 R2/R8";

bool bleStackInitialized = false;
bool deviceInformationReady = false;
bool advertisingActive = false;
bool policyReported = false;
uint32_t advertisingRestartAfterMs = 0;

BLEServer *bleServer = nullptr;
BLEService *deviceInformationService = nullptr;

char deviceId[kDeviceIdLength + 1] = "FWP-000000";
char activeAdvertisingName[kMaxLegacyLocalNameLength + 1] = "";

void buildDeviceId() {
  uint8_t mac[6] = {0};

  if (esp_read_mac(mac, ESP_MAC_WIFI_STA) == ESP_OK) {
    snprintf(
        deviceId,
        sizeof(deviceId),
        "FWP-%02X%02X%02X",
        mac[3],
        mac[4],
        mac[5]);
    return;
  }

  snprintf(deviceId, sizeof(deviceId), "FWP-000000");
}

void buildAdvertisingName(char *out, size_t outSize) {
  if (out == nullptr || outSize == 0) {
    return;
  }

  char safeAlias[kFwDeviceAliasMaxLen + 1] = "";
  const char *alias = fwDeviceAlias();
  size_t safeLength = 0;

  if (alias != nullptr) {
    for (size_t i = 0;
         alias[i] != '\0' && safeLength < kFwDeviceAliasMaxLen;
         ++i) {
      const unsigned char c = static_cast<unsigned char>(alias[i]);
      safeAlias[safeLength++] =
          (c >= 0x20 && c <= 0x7e) ? static_cast<char>(c) : '?';
    }
  }
  safeAlias[safeLength] = '\0';

  const size_t prefixLength = strlen(deviceId) + 1;
  const size_t aliasLimit =
      prefixLength < kMaxLegacyLocalNameLength
          ? kMaxLegacyLocalNameLength - prefixLength
          : 0;

  snprintf(
      out,
      outSize,
      "%s %.*s",
      deviceId,
      static_cast<int>(aliasLimit),
      safeAlias);
}

bool restartDelayElapsed() {
  return static_cast<int32_t>(millis() - advertisingRestartAfterMs) >= 0;
}

void setReadOnlyTextCharacteristic(
    BLEService *service,
    uint16_t uuid,
    const char *value) {
  BLECharacteristic *characteristic = service->createCharacteristic(
      BLEUUID(uuid),
      BLECharacteristic::PROPERTY_READ);
  characteristic->setValue(std::string(value == nullptr ? "" : value));
}

void ensureDeviceInformationService(Stream &out) {
  if (deviceInformationReady) {
    return;
  }

  bleServer = BLEDevice::createServer();
  deviceInformationService = bleServer->createService(
      BLEUUID(kDeviceInformationServiceUuid));

  setReadOnlyTextCharacteristic(
      deviceInformationService,
      kManufacturerNameUuid,
      kManufacturerName);
  setReadOnlyTextCharacteristic(
      deviceInformationService,
      kModelNumberUuid,
      kModelNumber);
  setReadOnlyTextCharacteristic(
      deviceInformationService,
      kSerialNumberUuid,
      deviceId);
  setReadOnlyTextCharacteristic(
      deviceInformationService,
      kFirmwareRevisionUuid,
      kFirmwareRevision);
  setReadOnlyTextCharacteristic(
      deviceInformationService,
      kHardwareRevisionUuid,
      kHardwareRevision);

  deviceInformationService->start();
  deviceInformationReady = true;

  out.print("[ble] Device Information service ready manufacturer=\"");
  out.print(kManufacturerName);
  out.print("\" model=\"");
  out.print(kModelNumber);
  out.print("\" serial=\"");
  out.print(deviceId);
  out.print("\" firmware=\"");
  out.print(kFirmwareRevision);
  out.print("\" hardware=\"");
  out.print(kHardwareRevision);
  out.println("\"");
}

void stopAdvertising(Stream &out, const char *reason) {
  if (!advertisingActive) {
    return;
  }

  BLEDevice::getAdvertising()->stop();
  advertisingActive = false;
  activeAdvertisingName[0] = '\0';

  out.print("[ble] discovery advertising stopped");
  if (reason != nullptr && reason[0] != '\0') {
    out.print(" reason=");
    out.print(reason);
  }
  out.println();
}

void startAdvertising(Stream &out, const char *name) {
  if (!bleStackInitialized) {
    BLEDevice::init("FarmWhisper");
    bleStackInitialized = true;
    out.println("[ble] stack initialized");
  }

  ensureDeviceInformationService(out);

  BLEAdvertisementData advertisingData;
  advertisingData.setFlags(
      ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT);
  advertisingData.setName(std::string(name));
  advertisingData.setCompleteServices(
      BLEUUID(kDeviceInformationServiceUuid));

  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  advertising->setScanResponse(false);
  advertising->setAdvertisementData(advertisingData);
  advertising->setAdvertisementType(ADV_TYPE_IND);
  advertising->start();

  snprintf(
      activeAdvertisingName,
      sizeof(activeAdvertisingName),
      "%s",
      name);
  advertisingActive = true;

  out.print("[ble] discovery advertising name=\"");
  out.print(activeAdvertisingName);
  out.print("\" deviceId=");
  out.print(deviceId);
  out.print(" alias=\"");
  out.print(fwDeviceAlias());
  out.println("\" service=180A readOnly=yes customServices=none");
}

void reconcile(Stream &out, bool reportPolicy) {
  const FwTransportModeId mode = fwSelectedTransportModeId();
  const bool shouldAdvertise = fwTransportModeUsesBluetoothLe(mode);

  if (!shouldAdvertise) {
    advertisingRestartAfterMs = 0;
    stopAdvertising(out, "transport-mode");

    if (reportPolicy || !policyReported) {
      out.print("[ble] discovery disabled by transport mode ");
      out.println(fwTransportModeName(mode));
    }

    policyReported = true;
    return;
  }

  char desiredName[kMaxLegacyLocalNameLength + 1] = "";
  buildAdvertisingName(desiredName, sizeof(desiredName));

  if (advertisingActive &&
      strcmp(activeAdvertisingName, desiredName) != 0) {
    stopAdvertising(out, "identity-change");
    advertisingRestartAfterMs = millis() + kAdvertisingRestartDelayMs;
  }

  if (!advertisingActive && restartDelayElapsed()) {
    startAdvertising(out, desiredName);
  }

  policyReported = true;
}

} // namespace

namespace FWBLE {

void begin(Stream &out) {
  buildDeviceId();
  reconcile(out, true);
}

void service(Stream &out) {
  reconcile(out, false);
}

bool isAdvertising() {
  return advertisingActive;
}

const char *advertisedName() {
  return activeAdvertisingName;
}

} // namespace FWBLE
