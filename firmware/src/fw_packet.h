#pragma once

#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>

namespace FWPacket {

constexpr uint8_t kProtocolVersion = 1;
constexpr uint8_t kTelemetryType = 1;
constexpr size_t kTelemetryPacketSize = 29;

constexpr uint16_t kFlagTofValid = 1U << 0;
constexpr uint16_t kFlagTofStable = 1U << 1;
constexpr uint16_t kFlagCalibrated = 1U << 2;
constexpr uint16_t kUnknownU16 = 0xFFFFU;

struct Telemetry {
  uint16_t flags;
  uint32_t sequence;
  uint8_t sourceId[3];
  uint16_t distanceMm;
  uint16_t emptyMm;
  uint16_t fullMm;
  uint16_t fillPermille;
  uint16_t batteryMillivolts;
  uint32_t uptimeSeconds;
};

bool encodeTelemetry(
    const Telemetry &telemetry,
    uint8_t *output,
    size_t outputSize,
    size_t &encodedSize);

bool decodeTelemetry(
    const uint8_t *data,
    size_t dataSize,
    Telemetry &telemetry);

void readLocalSourceId(uint8_t sourceId[3]);
void formatSourceId(
    const uint8_t sourceId[3],
    char *destination,
    size_t destinationSize);

uint16_t calculateFillPermille(
    uint16_t distanceMm,
    uint16_t emptyMm,
    uint16_t fullMm);

}  // namespace FWPacket
