#include "fw_ble_telemetry.h"

namespace {

constexpr uint8_t kCompanyIdLow = 0xFF;
constexpr uint8_t kCompanyIdHigh = 0xFF;
constexpr uint8_t kMagic0 = 'F';
constexpr uint8_t kMagic1 = 'W';

void writeU16(uint8_t *destination, uint16_t value) {
  destination[0] = static_cast<uint8_t>(value & 0xFFU);
  destination[1] = static_cast<uint8_t>((value >> 8) & 0xFFU);
}

uint16_t readU16(const uint8_t *source) {
  return static_cast<uint16_t>(source[0]) |
      (static_cast<uint16_t>(source[1]) << 8);
}

}  // namespace

namespace FWBleTelemetry {

bool decodeManufacturerData(
    const uint8_t *data,
    size_t dataSize,
    FWPacket::Telemetry &telemetry) {
  if (data == nullptr || dataSize != kManufacturerDataSize) {
    return false;
  }

  if (data[0] != kCompanyIdLow || data[1] != kCompanyIdHigh ||
      data[2] != kMagic0 || data[3] != kMagic1 ||
      data[4] != FWPacket::kProtocolVersion) {
    return false;
  }

  telemetry = {};
  telemetry.flags = data[5];
  telemetry.sourceId[0] = data[6];
  telemetry.sourceId[1] = data[7];
  telemetry.sourceId[2] = data[8];
  telemetry.sequence = readU16(data + 9);
  telemetry.distanceMm = readU16(data + 11);
  telemetry.emptyMm = FWPacket::kUnknownU16;
  telemetry.fullMm = FWPacket::kUnknownU16;
  telemetry.fillPermille = readU16(data + 13);
  telemetry.batteryMillivolts = readU16(data + 15);
  telemetry.uptimeSeconds = 0;
  return true;
}

bool encodeManufacturerData(
    const FWPacket::Telemetry &telemetry,
    uint8_t *output,
    size_t outputSize,
    size_t &encodedSize) {
  encodedSize = 0;
  if (output == nullptr || outputSize < kManufacturerDataSize ||
      telemetry.sequence > 0xFFFFUL || telemetry.flags > 0x00FFU) {
    return false;
  }

  output[0] = kCompanyIdLow;
  output[1] = kCompanyIdHigh;
  output[2] = kMagic0;
  output[3] = kMagic1;
  output[4] = FWPacket::kProtocolVersion;
  output[5] = static_cast<uint8_t>(telemetry.flags);
  output[6] = telemetry.sourceId[0];
  output[7] = telemetry.sourceId[1];
  output[8] = telemetry.sourceId[2];
  writeU16(output + 9, static_cast<uint16_t>(telemetry.sequence));
  writeU16(output + 11, telemetry.distanceMm);
  writeU16(output + 13, telemetry.fillPermille);
  writeU16(output + 15, telemetry.batteryMillivolts);
  encodedSize = kManufacturerDataSize;
  return true;
}

}  // namespace FWBleTelemetry
