#pragma once

#include <stddef.h>
#include <stdint.h>

enum class FwTransportModeId : uint8_t {
  LoRa = 0,
  BluetoothLe = 1,
  LoRaAndBluetoothLe = 2,
};

struct FwTransportMode {
  FwTransportModeId id;
  const char *key;
  const char *name;
};

size_t fwTransportModeCount();
const FwTransportMode *fwTransportModeAt(size_t index);
const FwTransportMode *fwDefaultTransportMode();
const FwTransportMode *fwTransportModeById(FwTransportModeId id);
const FwTransportMode *fwTransportModeByKey(const char *key);
const char *fwTransportModeKey(FwTransportModeId id);
const char *fwTransportModeName(FwTransportModeId id);

bool fwTransportModeUsesLoRa(FwTransportModeId id);
bool fwTransportModeUsesBluetoothLe(FwTransportModeId id);
