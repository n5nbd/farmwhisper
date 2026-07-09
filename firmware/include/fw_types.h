#pragma once

/*
 * Shared FarmWhisper component-validation types.
 *
 * Keep this file limited to small cross-module data shapes and enums. Module-
 * specific implementation state should stay inside the module that owns it.
 */


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
