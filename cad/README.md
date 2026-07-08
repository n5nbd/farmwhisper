# CAD plan

This directory is reserved for FarmWhisper CAD planning and future source design notes. No CAD source files are created or modified in this pass.

## CAD source policy

- SCAD is the source of truth for FarmWhisper mechanical designs.
- Do not commit generated files such as .stl, .3mf, .gcode, or slicer/machine outputs.
- Builders can export printable files themselves from SCAD when they need them.

## Part family vision

The FW100 part family should eventually contain:

- a body
- a top cap
- a conical cap
- an MCU/MPU carrier
- a deck retainer
- sensor and deck parts

## Tolerancing and reuse

- Use shared tolerances where possible.
- The baseline for plastic-to-plastic interfaces is fitSlop = 0.30 mm.
- The baseline for threaded interfaces is threadSlop = 0.30 mm.
- Prefer a future single master/common SCAD source or shared include file for common dimensions and tolerances.

## Design assumptions

- Keep mechanical assumptions display-independent unless a part is specifically for a known node.
- Keep mechanical assumptions product-independent unless a part is explicitly tied to a specific product design.

## Notes

- This pass is documentation-only.
- No .scad files are created or modified yet.
