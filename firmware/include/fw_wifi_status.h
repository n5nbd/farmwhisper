#pragma once

#include <Arduino.h>

namespace FWWiFiStatus {

void begin();
void printStatus(Stream &out);
void scanOnce(Stream &out);

} // namespace FWWiFiStatus
