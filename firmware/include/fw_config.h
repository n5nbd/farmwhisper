#pragma once

#include <Arduino.h>

namespace FWConfig {

static constexpr uint32_t SerialBaud = 115200;

static constexpr uint32_t ButtonDebounceMs = 35;
static constexpr uint32_t ButtonLongPressMs = 1200;
static constexpr uint32_t ButtonMultiPressGapMs = 450;

static constexpr uint32_t ButtonShortFlashMs = 150;
static constexpr uint32_t ButtonLongFlashMs = 450;
static constexpr uint32_t ButtonMultiFlashMs = 350;

static constexpr uint32_t HeartbeatMs = 1000;
static constexpr uint32_t TofPollMs = 100;

static constexpr uint8_t StabilityWindowSize = 5;
static constexpr uint16_t StabilityMaxSpanMm = 25;

}  // namespace FWConfig
