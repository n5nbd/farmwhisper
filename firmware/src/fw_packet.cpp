#include "fw_packet.h"

#include <esp_mac.h>
#include <stdio.h>

namespace {

constexpr uint8_t kMagic0 = 'F';
constexpr uint8_t kMagic1 = 'W';

void writeU16(uint8_t *destination, uint16_t value) {
  destination[0] = static_cast<uint8_t>(value & 0xFFU);
  destination[1] = static_cast<uint8_t>((value >> 8) & 0xFFU);
}

void writeU32(uint8_t *destination, uint32_t value) {
  destination[0] = static_cast<uint8_t>(value & 0xFFUL);
  destination[1] = static_cast<uint8_t>((value >> 8) & 0xFFUL);
  destination[2] = static_cast<uint8_t>((value >> 16) & 0xFFUL);
  destination[3] = static_cast<uint8_t>((value >> 24) & 0xFFUL);
}

uint16_t readU16(const uint8_t *source) {
  return static_cast<uint16_t>(source[0]) |
      (static_cast<uint16_t>(source[1]) << 8);
}

uint32_t readU32(const uint8_t *source) {
  return static_cast<uint32_t>(source[0]) |
      (static_cast<uint32_t>(source[1]) << 8) |
      (static_cast<uint32_t>(source[2]) << 16) |
      (static_cast<uint32_t>(source[3]) << 24);
}

uint16_t crc16Ccitt(const uint8_t *data, size_t length) {
  uint16_t crc = 0xFFFFU;

  for (size_t i = 0; i < length; ++i) {
    crc ^= static_cast<uint16_t>(data[i]) << 8;
    for (uint8_t bit = 0; bit < 8; ++bit) {
      crc = (crc & 0x8000U) != 0
          ? static_cast<uint16_t>((crc << 1) ^ 0x1021U)
          : static_cast<uint16_t>(crc << 1);
    }
  }

  return crc;
}

}  // namespace

namespace FWPacket {

bool encodeTelemetry(
    const Telemetry &telemetry,
    uint8_t *output,
    size_t outputSize,
    size_t &encodedSize) {
  encodedSize = 0;
  if (output == nullptr || outputSize < kTelemetryPacketSize) {
    return false;
  }

  output[0] = kMagic0;
  output[1] = kMagic1;
  output[2] = kProtocolVersion;
  output[3] = kTelemetryType;
  writeU16(output + 4, telemetry.flags);
  writeU32(output + 6, telemetry.sequence);
  output[10] = telemetry.sourceId[0];
  output[11] = telemetry.sourceId[1];
  output[12] = telemetry.sourceId[2];
  writeU16(output + 13, telemetry.distanceMm);
  writeU16(output + 15, telemetry.emptyMm);
  writeU16(output + 17, telemetry.fullMm);
  writeU16(output + 19, telemetry.fillPermille);
  writeU16(output + 21, telemetry.batteryMillivolts);
  writeU32(output + 23, telemetry.uptimeSeconds);

  writeU16(output + 27, crc16Ccitt(output, 27));
  encodedSize = kTelemetryPacketSize;
  return true;
}

bool decodeTelemetry(
    const uint8_t *data,
    size_t dataSize,
    Telemetry &telemetry) {
  if (data == nullptr || dataSize != kTelemetryPacketSize) {
    return false;
  }

  if (data[0] != kMagic0 || data[1] != kMagic1 ||
      data[2] != kProtocolVersion || data[3] != kTelemetryType) {
    return false;
  }

  const uint16_t expectedCrc = readU16(data + 27);
  if (crc16Ccitt(data, 27) != expectedCrc) {
    return false;
  }

  telemetry.flags = readU16(data + 4);
  telemetry.sequence = readU32(data + 6);
  telemetry.sourceId[0] = data[10];
  telemetry.sourceId[1] = data[11];
  telemetry.sourceId[2] = data[12];
  telemetry.distanceMm = readU16(data + 13);
  telemetry.emptyMm = readU16(data + 15);
  telemetry.fullMm = readU16(data + 17);
  telemetry.fillPermille = readU16(data + 19);
  telemetry.batteryMillivolts = readU16(data + 21);
  telemetry.uptimeSeconds = readU32(data + 23);
  return true;
}

void readLocalSourceId(uint8_t sourceId[3]) {
  uint8_t mac[6] = {};
  if (sourceId == nullptr) {
    return;
  }

  if (esp_read_mac(mac, ESP_MAC_WIFI_STA) != ESP_OK) {
    sourceId[0] = 0;
    sourceId[1] = 0;
    sourceId[2] = 0;
    return;
  }

  sourceId[0] = mac[3];
  sourceId[1] = mac[4];
  sourceId[2] = mac[5];
}

void formatSourceId(
    const uint8_t sourceId[3],
    char *destination,
    size_t destinationSize) {
  if (destination == nullptr || destinationSize == 0) {
    return;
  }

  if (sourceId == nullptr) {
    snprintf(destination, destinationSize, "FWP-000000");
    return;
  }

  snprintf(
      destination,
      destinationSize,
      "FWP-%02X%02X%02X",
      sourceId[0],
      sourceId[1],
      sourceId[2]);
}

uint16_t calculateFillPermille(
    uint16_t distanceMm,
    uint16_t emptyMm,
    uint16_t fullMm) {
  if (distanceMm == kUnknownU16 || emptyMm == kUnknownU16 ||
      fullMm == kUnknownU16 || emptyMm <= fullMm) {
    return kUnknownU16;
  }

  if (distanceMm >= emptyMm) {
    return 0;
  }
  if (distanceMm <= fullMm) {
    return 1000;
  }

  const uint32_t usableSpan = static_cast<uint32_t>(emptyMm - fullMm);
  const uint32_t filledSpan = static_cast<uint32_t>(emptyMm - distanceMm);
  return static_cast<uint16_t>((filledSpan * 1000UL) / usableSpan);
}

}  // namespace FWPacket
