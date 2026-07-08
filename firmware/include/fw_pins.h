#pragma once

// FarmWhisper pin contract and hardware notes.
//
// The display I2C pins are reserved for display use only and must not be reused for
// other peripherals.
//
// The onboard GPIO35 white LED is not a product status indicator and should not be used.
//
// Display support is optional and disabled by default. The firmware should not assume
// that display hardware is present in production.

// Future board-specific pin definitions and reserved pin mappings should be added here.
