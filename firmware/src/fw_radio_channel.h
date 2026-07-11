#pragma once

#include <stddef.h>
#include <stdint.h>

// FarmWhisper US radio channel plan.
//
// Channels are firmware-defined and presented to users as named selections.
// Raw frequency entry is intentionally not exposed.

struct FwRadioChannel {
  uint8_t number;
  const char *key;
  const char *name;
  uint32_t frequencyHz;
};

size_t fwRadioChannelCount();
const FwRadioChannel *fwRadioChannelAt(size_t index);

const FwRadioChannel *fwDefaultRadioChannel();
const FwRadioChannel *fwRadioChannelByNumber(uint8_t number);
const FwRadioChannel *fwRadioChannelByKey(const char *key);

const char *fwRadioChannelKey(uint8_t number);
const char *fwRadioChannelName(uint8_t number);
uint32_t fwRadioChannelFrequencyHz(uint8_t number);
