#if defined(FW_BOARD_XIAO_C6)

#include "fw_battery.h"

namespace FWBattery {

void begin() {}

uint16_t readMillivolts() {
  return kUnknownMillivolts;
}

void printStatus(Stream &out) {
  out.println("[battery] voltage=unknown");
}

}  // namespace FWBattery

#endif
