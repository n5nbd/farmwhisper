# FarmWhisper serial diagnostics

The component-validation firmware exposes a small serial command interface for hardware bring-up and regression testing.

## Commands

| Command | Purpose |
| --- | --- |
| `h` or `?` | Print help |
| `s` | Print status snapshot |
| `i` | Scan product I2C bus |
| `g` | Print expansion GPIO smoke status |
| `v` | Toggle verbose per-sample ToF logging |
| `x` | Print WiFi radio status without scanning |
| `w` | Run one manual WiFi scan, then return WiFi OFF |
| `r` | Reset runtime diagnostic counters |

## WiFi command contract

`x` is read-only. It must not enable STA mode, start AP mode, scan, connect, or modify stored settings.

`w` is temporary. It may enable STA mode only long enough to perform a manual scan. It must force WiFi back OFF before returning.

## Regression rule

Adding a new command must not change behavior of existing commands or boot-time component validation.
