#include "fw_transport_mode.h"

#include <cstring>

namespace {

constexpr FwTransportMode kTransportModes[] = {
    {
        FwTransportModeId::LoRa,
        "lora",
        "LoRa",
    },
    {
        FwTransportModeId::BluetoothLe,
        "bluetooth-le",
        "Bluetooth LE",
    },
    {
        FwTransportModeId::LoRaAndBluetoothLe,
        "lora-bluetooth-le",
        "LoRa + Bluetooth LE",
    },
};

constexpr size_t kTransportModeCount =
    sizeof(kTransportModes) / sizeof(kTransportModes[0]);

}  // namespace

size_t fwTransportModeCount() {
  return kTransportModeCount;
}

const FwTransportMode *fwTransportModeAt(size_t index) {
  if (index >= kTransportModeCount) {
    return nullptr;
  }

  return &kTransportModes[index];
}

const FwTransportMode *fwDefaultTransportMode() {
  return &kTransportModes[0];
}

const FwTransportMode *fwTransportModeById(FwTransportModeId id) {
  for (size_t i = 0; i < kTransportModeCount; ++i) {
    if (kTransportModes[i].id == id) {
      return &kTransportModes[i];
    }
  }

  return nullptr;
}

const FwTransportMode *fwTransportModeByKey(const char *key) {
  if (key == nullptr || key[0] == '\0') {
    return nullptr;
  }

  for (size_t i = 0; i < kTransportModeCount; ++i) {
    if (strcmp(kTransportModes[i].key, key) == 0) {
      return &kTransportModes[i];
    }
  }

  return nullptr;
}

const char *fwTransportModeKey(FwTransportModeId id) {
  const FwTransportMode *mode = fwTransportModeById(id);
  return mode == nullptr ? "" : mode->key;
}

const char *fwTransportModeName(FwTransportModeId id) {
  const FwTransportMode *mode = fwTransportModeById(id);
  return mode == nullptr ? "" : mode->name;
}

bool fwTransportModeUsesLoRa(FwTransportModeId id) {
  return id == FwTransportModeId::LoRa ||
         id == FwTransportModeId::LoRaAndBluetoothLe;
}

bool fwTransportModeUsesBluetoothLe(FwTransportModeId id) {
  return id == FwTransportModeId::BluetoothLe ||
         id == FwTransportModeId::LoRaAndBluetoothLe;
}
