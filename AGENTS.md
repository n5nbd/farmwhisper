# Agent guidance for FarmWhisper

- Keep this repository clean and source-focused.
- Prefer small, reviewable changes over large rewrites.
- Preserve the existing project structure under cad/, docs/, firmware/, hardware/, tools/, and tests/.
- Use the documents in docs/ as the starting point for design decisions and implementation notes.
- The current development-base board is the Heltec WiFi LoRa 32 V4.
- The built-in display is allowed for development and debugging, but production firmware must remain display-optional.
- The display I2C pins are reserved for display use only and must not be reused for other peripherals.
- The first product target is the coop feed sensor using the Heltec V4 dev base, a single NeoPixel, and a big button.
- The onboard GPIO35 white LED is not a product status indicator and should not be used.
- CAD source policy: commit .scad only; do not commit .stl, .3mf, .gcode, or slicer/machine outputs.
- FW100 CAD tolerance baseline: use fitSlop = 0.30 mm for plastic-to-plastic contact and threadSlop = 0.30 mm for threaded interfaces.
- FarmWhisper UI convention: prefix questionable, stale, or unstable readings with ~.
- Keep this pass documentation-only; do not add firmware sources yet.
