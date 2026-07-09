#include "fw_device_config.h"

namespace {
constexpr const char* kDefaultDeviceAlias = "FarmWhisper device";
}

const char* fwDeviceAlias() {
  return kDefaultDeviceAlias;
}
