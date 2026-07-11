#include "fw_radio_channel.h"

#include <string.h>

namespace {

constexpr FwRadioChannel kRadioChannels[] = {
    {1, "ch01", "Channel 1 - 905.5 MHz", 905500000UL},
    {2, "ch02", "Channel 2 - 906.5 MHz", 906500000UL},
    {3, "ch03", "Channel 3 - 907.5 MHz", 907500000UL},
    {4, "ch04", "Channel 4 - 908.5 MHz", 908500000UL},
    {5, "ch05", "Channel 5 - 909.5 MHz", 909500000UL},
    {6, "ch06", "Channel 6 - 910.5 MHz", 910500000UL},
    {7, "ch07", "Channel 7 - 911.5 MHz", 911500000UL},
    {8, "ch08", "Channel 8 - 912.5 MHz", 912500000UL},
    {9, "ch09", "Channel 9 - 913.5 MHz", 913500000UL},
    {10, "ch10", "Channel 10 - 914.5 MHz", 914500000UL},
    {11, "ch11", "Channel 11 - 915.5 MHz", 915500000UL},
    {12, "ch12", "Channel 12 - 916.5 MHz", 916500000UL},
    {13, "ch13", "Channel 13 - 917.5 MHz", 917500000UL},
};

constexpr size_t kRadioChannelCount =
    sizeof(kRadioChannels) / sizeof(kRadioChannels[0]);
constexpr uint8_t kDefaultRadioChannelNumber = 9;

}  // namespace

size_t fwRadioChannelCount() {
  return kRadioChannelCount;
}

const FwRadioChannel *fwRadioChannelAt(size_t index) {
  if (index >= kRadioChannelCount) {
    return nullptr;
  }

  return &kRadioChannels[index];
}

const FwRadioChannel *fwDefaultRadioChannel() {
  return fwRadioChannelByNumber(kDefaultRadioChannelNumber);
}

const FwRadioChannel *fwRadioChannelByNumber(uint8_t number) {
  for (size_t i = 0; i < kRadioChannelCount; ++i) {
    if (kRadioChannels[i].number == number) {
      return &kRadioChannels[i];
    }
  }

  return nullptr;
}

const FwRadioChannel *fwRadioChannelByKey(const char *key) {
  if (key == nullptr || key[0] == '\0') {
    return nullptr;
  }

  for (size_t i = 0; i < kRadioChannelCount; ++i) {
    if (strcmp(kRadioChannels[i].key, key) == 0) {
      return &kRadioChannels[i];
    }
  }

  return nullptr;
}

const char *fwRadioChannelKey(uint8_t number) {
  const FwRadioChannel *channel = fwRadioChannelByNumber(number);
  return channel == nullptr ? "" : channel->key;
}

const char *fwRadioChannelName(uint8_t number) {
  const FwRadioChannel *channel = fwRadioChannelByNumber(number);
  return channel == nullptr ? "" : channel->name;
}

uint32_t fwRadioChannelFrequencyHz(uint8_t number) {
  const FwRadioChannel *channel = fwRadioChannelByNumber(number);
  return channel == nullptr ? 0 : channel->frequencyHz;
}
