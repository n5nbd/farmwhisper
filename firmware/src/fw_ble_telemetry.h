#pragma once

#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>

#include "fw_packet.h"

namespace FWBleTelemetry {

// Manufacturer data includes the two-byte company identifier used by the
// current FarmWhisper sensor advertisement.
constexpr size_t kManufacturerDataSize = 17;

bool decodeManufacturerData(
    const uint8_t *data,
    size_t dataSize,
    FWPacket::Telemetry &telemetry);

bool encodeManufacturerData(
    const FWPacket::Telemetry &telemetry,
    uint8_t *output,
    size_t outputSize,
    size_t &encodedSize);

}  // namespace FWBleTelemetry
