#pragma once

enum class ComponentStatus {
  Booting,
  TofInitFailed,
  TofTimeout,
  TofShady,
  TofWarming,
  TofUnstable,
  TofStable
};

enum class ButtonOverlay {
  None,
  ShortPress,
  LongPress,
  DoublePress,
  TriplePress
};
