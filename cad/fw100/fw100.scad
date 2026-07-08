// FarmWhisper FW100 master CAD source
// Units: millimeters
//
// Clean modular master: body, caps, nut, load spreader, retainer, and decks.
// Requires BOSL2 installed in OpenSCAD user libraries:
//   ~/.local/share/OpenSCAD/libraries/BOSL2
//
// Repo/export convention:
//   Keep this .scad source in the repo. Export STL/3MF/G-code locally as needed.

include <BOSL2/std.scad>
include <BOSL2/threading.scad>

$fn = 160;
epsilon = 0.05;

// -----------------------------------------------------------------------------
// Render selector
// -----------------------------------------------------------------------------
// Pick one:
//   "body"
//   "cap_blank"
//   "cap_conical"
//   "nut"
//   "load_spreader"
//   "retainer"
//   "deck_tof"
//   "deck_notched"
//   "deck_mpu"
//   "layout"
part = "layout";

// -----------------------------------------------------------------------------
// Global printer/material tuning
// -----------------------------------------------------------------------------
// For anything that touches another printed plastic part, clearance is radial
// unless the local dimension says otherwise.
fitSlop = 0.30;
slop = fitSlop;
threadSlop = 0.30;
$slop = threadSlop;

// -----------------------------------------------------------------------------
// Shared FW100 body dimensions
// -----------------------------------------------------------------------------
bodyOd = 100;
innerBoreId = 80;
mainBodyHeight = 50;

// Bottom support lip at the bottom of the lower/mounting thread.
supportLipThickness = 3;
supportLipWidth = 3;
supportHoleId = innerBoreId - (2 * supportLipWidth);

// -----------------------------------------------------------------------------
// Shared FW100 trapezoidal thread definition
// -----------------------------------------------------------------------------
threadOd = 92;          // major diameter: outside diameter at thread crests
threadPitch = 6;        // axial distance from one thread to the next
threadDepth = 2;        // radial distance from root to crest

threadTurnsModule = 2;
threadTurnsMount = 5;

threadHeightModule = threadPitch * threadTurnsModule;
threadHeightMount = threadPitch * threadTurnsMount;

threadChamferHeight = 6;
threadChamferOverlap = 3;

// Derived body Z locations.
bodyBottomZ = -threadChamferHeight - threadHeightMount;
bodyTopZ = mainBodyHeight + threadHeightModule;
supportLipZ = bodyBottomZ;

// -----------------------------------------------------------------------------
// Shared helpers
// -----------------------------------------------------------------------------
module fw100Tube(outerD, innerD, h) {
    difference() {
        cylinder(d = outerD, h = h);
        translate([0, 0, -0.1])
            cylinder(d = innerD, h = h + 0.2);
    }
}

module fw100ExternalThread(height) {
    trapezoidal_threaded_rod(
        d = threadOd,
        l = height,
        pitch = threadPitch,
        thread_depth = threadDepth,
        anchor = BOTTOM
    );
}

module fw100InternalThreadMask(height, femaleD, femaleDepth) {
    // Internal-thread mask with BOSL2 start/end treatments disabled.
    // Final faces are usually produced by slicing or mouth chamfers, not by
    // relying on the mask endpoints as final printable thread geometry.
    trapezoidal_threaded_rod(
        d = femaleD,
        l = height,
        pitch = threadPitch,
        thread_depth = femaleDepth,
        internal = true,
        blunt_start = false,
        bevel = false,
        end_len = 0,
        lead_in = 0,
        anchor = BOTTOM
    );
}

module fw100ChamferedDisc(d, h, chamfer) {
    if (chamfer <= 0) {
        cylinder(d = d, h = h);
    } else {
        union() {
            translate([0, 0, chamfer])
                cylinder(d = d, h = h - (2 * chamfer));

            cylinder(d1 = d - (2 * chamfer), d2 = d, h = chamfer);

            translate([0, 0, h - chamfer])
                cylinder(d1 = d, d2 = d - (2 * chamfer), h = chamfer);
        }
    }
}

module fw100ChamferedRoundThroughHole(d, h, chamfer) {
    eps = 0.1;

    union() {
        translate([0, 0, -eps])
            cylinder(d = d, h = h + (2 * eps));

        if (chamfer > 0) {
            translate([0, 0, -eps])
                cylinder(d1 = d + (2 * chamfer), d2 = d, h = chamfer + eps);

            translate([0, 0, h - chamfer])
                cylinder(d1 = d, d2 = d + (2 * chamfer), h = chamfer + eps);
        }
    }
}

module fw100RectSlice(x, y, z, t = 0.01) {
    translate([0, 0, z])
        cube([x, y, t], center = true);
}

module fw100ChamferedRectThroughHole(x, y, h, chamfer) {
    eps = 0.1;
    slice = 0.01;

    union() {
        translate([0, 0, h / 2])
            cube([x, y, h + (2 * eps)], center = true);

        if (chamfer > 0) {
            hull() {
                fw100RectSlice(x + (2 * chamfer), y + (2 * chamfer), -eps, slice);
                fw100RectSlice(x, y, chamfer, slice);
            }

            hull() {
                fw100RectSlice(x, y, h - chamfer, slice);
                fw100RectSlice(x + (2 * chamfer), y + (2 * chamfer), h + eps, slice);
            }
        }
    }
}

// -----------------------------------------------------------------------------
// Main body
// -----------------------------------------------------------------------------
module fw100Body() {
    difference() {
        union() {
            // Smooth main 100 mm body.
            cylinder(d = bodyOd, h = mainBodyHeight);

            // Top/module thread: no bevel/chamfer, just thread turns.
            translate([0, 0, mainBodyHeight])
                fw100ExternalThread(threadHeightModule);

            // Bottom chamfer: thread OD up to 100 mm body OD.
            translate([0, 0, -threadChamferHeight])
                cylinder(d1 = threadOd, d2 = bodyOd, h = threadChamferHeight);

            // Bottom mounting thread reaches up into the chamfer.
            translate([0, 0, bodyBottomZ])
                fw100ExternalThread(threadHeightMount + threadChamferOverlap);

            // Support lip/floor at the bottom of the lower thread.
            translate([0, 0, supportLipZ])
                fw100Tube(innerBoreId, supportHoleId, supportLipThickness);
        }

        // Full inner chamber bore runs down to the top of the support lip.
        translate([0, 0, supportLipZ + supportLipThickness])
            cylinder(
                d = innerBoreId,
                h = bodyTopZ - (supportLipZ + supportLipThickness) + 0.2
            );

        // Smaller center hole passes through only the support lip.
        translate([0, 0, supportLipZ - 0.1])
            cylinder(
                d = supportHoleId,
                h = supportLipThickness + 0.2
            );
    }
}

// -----------------------------------------------------------------------------
// Top cap
// -----------------------------------------------------------------------------
topCapOd = 106;
topCapThreadTurns = threadTurnsModule;
topCapThreadedSkirtHeight = topCapThreadTurns * threadPitch;
topCapThreadClearance = 3;
topCapRoofThickness = 4;
topCapHeight = topCapThreadedSkirtHeight + topCapThreadClearance + topCapRoofThickness;

topCapBottomOuterChamfer = 1.2;
topCapTopOuterChamfer = 4.0;
topCapEntryChamferHeight = 0.35;
topCapEntryChamferExtraDia = 1.0;

topCapKnurlCutDepth = 1.0;
topCapKnurlToothWidth = 1.15;
topCapKnurlCount = 44;
topCapKnurlAngle = 45;
topCapKnurlOverrunTurns = 0.5;
topCapKnurlBandBottom = -topCapKnurlOverrunTurns * threadPitch;
topCapKnurlBandTop = topCapHeight + (topCapKnurlOverrunTurns * threadPitch);
topCapKnurlBandHeight = topCapKnurlBandTop - topCapKnurlBandBottom;

topCapFemaleThreadOd = threadOd + (2 * threadSlop);
topCapFemaleThreadDepth = threadDepth + threadSlop;
topCapClearanceBoreId = topCapFemaleThreadOd + 1.0;

topCapThreadBottomOverrunTurns = 0.5;
topCapThreadCutterTurns = topCapThreadTurns + topCapThreadBottomOverrunTurns;
topCapThreadCutterHeight = topCapThreadCutterTurns * threadPitch;
topCapThreadCutterBottomZ = topCapThreadedSkirtHeight - topCapThreadCutterHeight;

module fw100TopCapThreadMask() {
    translate([0, 0, topCapThreadCutterBottomZ])
        fw100InternalThreadMask(
            topCapThreadCutterHeight,
            topCapFemaleThreadOd,
            topCapFemaleThreadDepth
        );
}

module fw100TopCapStraightOuterBlank() {
    hull() {
        translate([0, 0, topCapBottomOuterChamfer])
            cylinder(d = topCapOd, h = 0.01);
        translate([0, 0, topCapHeight - 0.01])
            cylinder(d = topCapOd, h = 0.01);
        translate([0, 0, 0])
            cylinder(d = topCapOd - (2 * topCapBottomOuterChamfer), h = 0.01);
    }
}

module fw100TopCapFinalTopChamferEnvelope() {
    hull() {
        translate([0, 0, 0])
            cylinder(d = topCapOd + 0.01, h = 0.01);
        translate([0, 0, topCapHeight - topCapTopOuterChamfer])
            cylinder(d = topCapOd + 0.01, h = 0.01);
        translate([0, 0, topCapHeight - 0.01])
            cylinder(d = topCapOd - (2 * topCapTopOuterChamfer), h = 0.01);
    }
}

module fw100TopCapHelicalGrooveCutter(twistDegrees) {
    translate([0, 0, topCapKnurlBandBottom])
        linear_extrude(
            height = topCapKnurlBandHeight,
            twist = twistDegrees,
            slices = max(48, ceil(topCapKnurlBandHeight * 4)),
            convexity = 8
        )
            polygon(points = [
                [topCapOd / 2 - topCapKnurlCutDepth, -topCapKnurlToothWidth / 2],
                [topCapOd / 2 + 0.85,                  -topCapKnurlToothWidth / 2],
                [topCapOd / 2 + 0.85,                   topCapKnurlToothWidth / 2],
                [topCapOd / 2 - topCapKnurlCutDepth,  topCapKnurlToothWidth / 2]
            ]);
}

module fw100TopCapSideKnurlCutters() {
    topCapKnurlTwistDegrees = 360 * topCapKnurlBandHeight / (PI * topCapOd * tan(topCapKnurlAngle));

    intersection() {
        union() {
            for (i = [0 : topCapKnurlCount - 1]) {
                rotate([0, 0, i * 360 / topCapKnurlCount])
                    fw100TopCapHelicalGrooveCutter(topCapKnurlTwistDegrees);

                rotate([0, 0, (i + 0.5) * 360 / topCapKnurlCount])
                    fw100TopCapHelicalGrooveCutter(-topCapKnurlTwistDegrees);
            }
        }

        translate([0, 0, topCapKnurlBandBottom])
            cylinder(d = topCapOd + 6, h = topCapKnurlBandHeight);
    }
}

module fw100TopCapInternallyFinishedStraightCap() {
    difference() {
        fw100TopCapStraightOuterBlank();

        fw100TopCapThreadMask();

        translate([0, 0, topCapThreadedSkirtHeight - 0.05])
            cylinder(
                d = topCapClearanceBoreId,
                h = topCapThreadClearance + 0.10
            );

        translate([0, 0, -0.05])
            cylinder(
                d1 = topCapFemaleThreadOd + topCapEntryChamferExtraDia,
                d2 = topCapFemaleThreadOd,
                h = topCapEntryChamferHeight + 0.05
            );
    }
}

module fw100TopCapKnurledStraightCap() {
    difference() {
        fw100TopCapInternallyFinishedStraightCap();
        fw100TopCapSideKnurlCutters();
    }
}

module fw100TopCap() {
    intersection() {
        fw100TopCapKnurledStraightCap();
        fw100TopCapFinalTopChamferEnvelope();
    }
}

// -----------------------------------------------------------------------------
// Conical cap
// -----------------------------------------------------------------------------
conicalCapOd = 106;
conicalCapThreadTurns = threadTurnsModule;
conicalCapThreadedSkirtHeight = conicalCapThreadTurns * threadPitch;
conicalCapThreadClearance = 3;
conicalCapRoofThickness = 4;
conicalCapConeBaseZ = conicalCapThreadedSkirtHeight + conicalCapThreadClearance + conicalCapRoofThickness;

conicalCapConeTruncateHeight = 10;
conicalCapConeTopRadius = conicalCapConeTruncateHeight;
conicalCapConeTopDia = 2 * conicalCapConeTopRadius;
conicalCapConeHeight = (conicalCapOd / 2) - conicalCapConeTopRadius;
conicalCapConeTopZ = conicalCapConeBaseZ + conicalCapConeHeight;
conicalCapTotalHeight = conicalCapConeTopZ;

conicalCapBottomOuterChamfer = 1.2;
conicalCapEntryChamferHeight = 0.35;
conicalCapEntryChamferExtraDia = 1.0;

conicalCapWallThickness = 3.0;
conicalCapHollowStartZ = conicalCapThreadedSkirtHeight + conicalCapThreadClearance;
conicalCapTopCapThickness = 3.0;

conicalCapKnurlCutDepth = .5;
conicalCapKnurlToothWidth = 3;
conicalCapKnurlCount = 25;
conicalCapKnurlAngle = 45;
conicalCapKnurlOverrunTurns = 0.5;
conicalCapKnurlBandBottom = -conicalCapKnurlOverrunTurns * threadPitch;
conicalCapKnurlBandTop = conicalCapConeBaseZ + (conicalCapKnurlOverrunTurns * threadPitch);
conicalCapKnurlBandHeight = conicalCapKnurlBandTop - conicalCapKnurlBandBottom;

conicalCapFemaleThreadOd = threadOd + (2 * threadSlop);
conicalCapFemaleThreadDepth = threadDepth + threadSlop;
conicalCapClearanceBoreId = conicalCapFemaleThreadOd + 1.0;

conicalCapThreadBottomOverrunTurns = 0.5;
conicalCapThreadCutterTurns = conicalCapThreadTurns + conicalCapThreadBottomOverrunTurns;
conicalCapThreadCutterHeight = conicalCapThreadCutterTurns * threadPitch;
conicalCapThreadCutterBottomZ = conicalCapThreadedSkirtHeight - conicalCapThreadCutterHeight;

function fw100ConicalCapOuterRadiusAtZ(z) =
    z <= conicalCapConeBaseZ ? conicalCapOd / 2 :
    z <= conicalCapConeTopZ ? (conicalCapOd / 2) - (z - conicalCapConeBaseZ) :
    conicalCapConeTopRadius;

module fw100ConicalCapThreadMask() {
    translate([0, 0, conicalCapThreadCutterBottomZ])
        fw100InternalThreadMask(
            conicalCapThreadCutterHeight,
            conicalCapFemaleThreadOd,
            conicalCapFemaleThreadDepth
        );
}

module fw100ConicalCapStraightOuterBlank() {
    hull() {
        translate([0, 0, conicalCapBottomOuterChamfer])
            cylinder(d = conicalCapOd, h = 0.01);
        translate([0, 0, conicalCapTotalHeight - 0.01])
            cylinder(d = conicalCapOd, h = 0.01);
        translate([0, 0, 0])
            cylinder(d = conicalCapOd - (2 * conicalCapBottomOuterChamfer), h = 0.01);
    }
}

module fw100ConicalCapOuterEnvelope() {
    union() {
        cylinder(d = conicalCapOd + 0.02, h = conicalCapConeBaseZ + 0.01);

        translate([0, 0, conicalCapConeBaseZ - 0.01])
            cylinder(
                d1 = conicalCapOd + 0.02,
                d2 = conicalCapConeTopDia + 0.02,
                h = conicalCapConeHeight + 0.02
            );
    }
}

module fw100ConicalCapHollowCavity() {
    conicalCapHollowConeTopZ = conicalCapConeTopZ - conicalCapTopCapThickness;
    conicalCapHollowConeTopDia = max(
        4,
        2 * (fw100ConicalCapOuterRadiusAtZ(conicalCapHollowConeTopZ) - conicalCapWallThickness)
    );

    union() {
        translate([0, 0, conicalCapHollowStartZ])
            cylinder(
                d = conicalCapOd - (2 * conicalCapWallThickness),
                h = (conicalCapConeBaseZ - conicalCapHollowStartZ) + 0.10
            );

        translate([0, 0, conicalCapConeBaseZ - 0.05])
            cylinder(
                d1 = conicalCapOd - (2 * conicalCapWallThickness),
                d2 = conicalCapHollowConeTopDia,
                h = (conicalCapHollowConeTopZ - conicalCapConeBaseZ) + 0.05
            );
    }
}

module fw100ConicalCapHelicalGrooveCutter(twistDegrees) {
    translate([0, 0, conicalCapKnurlBandBottom])
        linear_extrude(
            height = conicalCapKnurlBandHeight,
            twist = twistDegrees,
            slices = max(48, ceil(conicalCapKnurlBandHeight * 4)),
            convexity = 8
        )
            polygon(points = [
                [conicalCapOd / 2 - conicalCapKnurlCutDepth, -conicalCapKnurlToothWidth / 2],
                [conicalCapOd / 2 + 0.85,                     -conicalCapKnurlToothWidth / 2],
                [conicalCapOd / 2 + 0.85,                      conicalCapKnurlToothWidth / 2],
                [conicalCapOd / 2 - conicalCapKnurlCutDepth,  conicalCapKnurlToothWidth / 2]
            ]);
}

module fw100ConicalCapSideKnurlCutters() {
    conicalCapKnurlTwistDegrees = 360 * conicalCapKnurlBandHeight / (PI * conicalCapOd * tan(conicalCapKnurlAngle));

    intersection() {
        union() {
            for (i = [0 : conicalCapKnurlCount - 1]) {
                rotate([0, 0, i * 360 / conicalCapKnurlCount])
                    fw100ConicalCapHelicalGrooveCutter(conicalCapKnurlTwistDegrees);

                rotate([0, 0, (i + 0.5) * 360 / conicalCapKnurlCount])
                    fw100ConicalCapHelicalGrooveCutter(-conicalCapKnurlTwistDegrees);
            }
        }

        translate([0, 0, conicalCapKnurlBandBottom])
            cylinder(d = conicalCapOd + 6, h = conicalCapKnurlBandHeight);
    }
}

module fw100ConicalCapInternallyFinishedBlank() {
    difference() {
        fw100ConicalCapStraightOuterBlank();

        fw100ConicalCapThreadMask();

        translate([0, 0, conicalCapThreadedSkirtHeight - 0.05])
            cylinder(
                d = conicalCapClearanceBoreId,
                h = conicalCapThreadClearance + 0.10
            );

        fw100ConicalCapHollowCavity();

        translate([0, 0, -0.05])
            cylinder(
                d1 = conicalCapFemaleThreadOd + conicalCapEntryChamferExtraDia,
                d2 = conicalCapFemaleThreadOd,
                h = conicalCapEntryChamferHeight + 0.05
            );
    }
}

module fw100ConicalCapKnurledBlank() {
    difference() {
        fw100ConicalCapInternallyFinishedBlank();
        fw100ConicalCapSideKnurlCutters();
    }
}

module fw100ConicalCap() {
    intersection() {
        fw100ConicalCapKnurledBlank();
        fw100ConicalCapOuterEnvelope();
    }
}

// -----------------------------------------------------------------------------
// Retaining nut
// -----------------------------------------------------------------------------
nutOd = 118;
nutThreadTurns = 2;
nutThickness = nutThreadTurns * threadPitch;
nutOuterChamfer = 1.2;
nutSliceAllowanceTurns = 0.5;
nutDonorThreadTurns = nutThreadTurns + (2 * nutSliceAllowanceTurns);
nutDonorHeight = nutDonorThreadTurns * threadPitch;
nutSliceBottom = nutSliceAllowanceTurns * threadPitch;
nutSliceTop = nutSliceBottom + nutThickness;

nutFemaleThreadOd = threadOd + (2 * threadSlop);
nutFemaleThreadDepth = threadDepth + threadSlop;
nutEntryChamferHeight = 0.35;
nutEntryChamferExtraDia = 1.0;
nutFlatCount = 12;
nutFlatDepth = 2.0;
nutBig = nutOd + 40;

module fw100NutCleanOuterEnvelope() {
    hull() {
        translate([0, 0, nutOuterChamfer])
            cylinder(d = nutOd, h = 0.01);
        translate([0, 0, nutThickness - nutOuterChamfer])
            cylinder(d = nutOd, h = 0.01);
        translate([0, 0, 0])
            cylinder(d = nutOd - (2 * nutOuterChamfer), h = 0.01);
        translate([0, 0, nutThickness - 0.01])
            cylinder(d = nutOd - (2 * nutOuterChamfer), h = 0.01);
    }
}

module fw100NutFingerFlatCutters() {
    if (nutFlatCount > 0 && nutFlatDepth > 0) {
        for (i = [0 : nutFlatCount - 1]) {
            rotate([0, 0, i * 360 / nutFlatCount])
                translate([nutOd / 2 + 25 - nutFlatDepth, 0, nutThickness / 2])
                    cube([50, nutOd, nutThickness + 2], center = true);
        }
    }
}

module fw100NutDonorThreeTurnNut() {
    difference() {
        cylinder(d = nutOd, h = nutDonorHeight);
        fw100InternalThreadMask(nutDonorHeight, nutFemaleThreadOd, nutFemaleThreadDepth);
    }
}

module fw100NutSlicedMiddleOfDonor() {
    translate([0, 0, -nutSliceBottom])
        intersection() {
            fw100NutDonorThreeTurnNut();
            translate([-nutBig / 2, -nutBig / 2, nutSliceBottom])
                cube([nutBig, nutBig, nutThickness]);
        }
}

module fw100RetainingNut() {
    difference() {
        intersection() {
            fw100NutSlicedMiddleOfDonor();
            fw100NutCleanOuterEnvelope();
        }

        fw100NutFingerFlatCutters();

        translate([0, 0, nutThickness - nutEntryChamferHeight])
            cylinder(
                d1 = nutFemaleThreadOd,
                d2 = nutFemaleThreadOd + nutEntryChamferExtraDia,
                h = nutEntryChamferHeight + 0.05
            );

        translate([0, 0, -0.05])
            cylinder(
                d1 = nutFemaleThreadOd + nutEntryChamferExtraDia,
                d2 = nutFemaleThreadOd,
                h = nutEntryChamferHeight + 0.05
            );
    }
}

// -----------------------------------------------------------------------------
// Load spreaders
// -----------------------------------------------------------------------------
loadSpreaderOd = 118;
loadSpreaderThickness = 8;
loadSpreaderSeatDepth = threadChamferHeight;

loadSpreaderThreadClearanceId = threadOd + (2 * threadSlop);
loadSpreaderSeatTopId = bodyOd + (2 * fitSlop);

module fw100LoadSpreader() {
    difference() {
        cylinder(d = loadSpreaderOd, h = loadSpreaderThickness);

        translate([0, 0, -0.1])
            cylinder(d = loadSpreaderThreadClearanceId, h = loadSpreaderThickness + 0.2);

        translate([0, 0, loadSpreaderThickness - loadSpreaderSeatDepth])
            cylinder(
                d1 = loadSpreaderThreadClearanceId,
                d2 = loadSpreaderSeatTopId,
                h = loadSpreaderSeatDepth + 0.1
            );
    }
}


// -----------------------------------------------------------------------------
// Standard 20 mm retainer
// -----------------------------------------------------------------------------
retainerHeight = 20;
retainerWallThickness = 3;
retainerEdgeChamfer = 0.4;
retainerOd = innerBoreId - (2 * fitSlop);
retainerId = retainerOd - (2 * retainerWallThickness);

assert(fitSlop >= 0, "fitSlop must be zero or positive.");
assert(retainerWallThickness > 0, "retainerWallThickness must be positive.");
assert(retainerId > 0, "retainerId must be positive. Check innerBoreId, fitSlop, and wall thickness.");
assert(retainerHeight > 2 * retainerEdgeChamfer, "retainerHeight must be greater than two edge chamfers.");

module fw100OuterChamferedCylinder(outerD, h, chamfer) {
    if (chamfer <= 0) {
        cylinder(d = outerD, h = h);
    } else {
        union() {
            translate([0, 0, chamfer])
                cylinder(d = outerD, h = h - (2 * chamfer));
            cylinder(d1 = outerD - (2 * chamfer), d2 = outerD, h = chamfer);
            translate([0, 0, h - chamfer])
                cylinder(d1 = outerD, d2 = outerD - (2 * chamfer), h = chamfer);
        }
    }
}

module fw100InnerBoreWithChamfers(innerD, h, chamfer) {
    union() {
        translate([0, 0, -epsilon])
            cylinder(d = innerD, h = h + (2 * epsilon));
        if (chamfer > 0) {
            translate([0, 0, -epsilon])
                cylinder(d1 = innerD + (2 * chamfer), d2 = innerD, h = chamfer + epsilon);
            translate([0, 0, h - chamfer])
                cylinder(d1 = innerD, d2 = innerD + (2 * chamfer), h = chamfer + epsilon);
        }
    }
}

module fw100Retainer() {
    difference() {
        fw100OuterChamferedCylinder(retainerOd, retainerHeight, retainerEdgeChamfer);
        fw100InnerBoreWithChamfers(retainerId, retainerHeight, retainerEdgeChamfer);
    }
}

// -----------------------------------------------------------------------------
// Deck shared dimensions and helpers
// -----------------------------------------------------------------------------
deckOd = innerBoreId - (2 * fitSlop);
deckThickness = 3.0;
deckFaceChamfer = 0.4;
deckHoleChamfer = 0.4;

module fw100DeckBlank() {
    fw100ChamferedDisc(deckOd, deckThickness, deckFaceChamfer);
}

// -----------------------------------------------------------------------------
// Deck-ToF: sensor board / connector deck
// -----------------------------------------------------------------------------
// Rectangular ToF connector slot. +X is right of center; long dimension is
// perpendicular to that offset line.
deckTofSlotX = 6;
deckTofSlotY = 18;
deckTofSlotOffsetX = 6;
deckTofSlotOffsetY = 0;

module fw100DeckToF() {
    difference() {
        fw100DeckBlank();
        translate([deckTofSlotOffsetX, deckTofSlotOffsetY, 0])
            fw100ChamferedRectThroughHole(
                deckTofSlotX,
                deckTofSlotY,
                deckThickness,
                deckHoleChamfer
            );
    }
}

// -----------------------------------------------------------------------------
// Deck-Notched: generic cable drop-in deck
// -----------------------------------------------------------------------------
// The cable keyway is driven by cableKeywayDia. Width = diameter, bottom radius
// = diameter/2. The straight sides are tangent to the round bottom and parallel
// to the center-to-edge reference line.
cableKeywayDia = 5.0;
cableKeywayEdgeRadiusFactor = 1.15;
cableKeywayOuterCornerRadius = 2.5;

module fw100CableKeywayCut2d() {
    deckR = deckOd / 2;
    cableR = cableKeywayDia / 2;
    cableHoleY = deckR - (cableKeywayEdgeRadiusFactor * cableR);
    mouthY = sqrt((deckR * deckR) - (cableR * cableR));

    union() {
        // U-shaped keyway: round bottom plus straight tangent sides.
        translate([0, cableHoleY])
            circle(r = cableR);
        translate([-cableR, cableHoleY])
            square([2 * cableR, deckOd], center = false);

        // Round the two material corners at the mouth of the notch.
        for (sx = [-1, 1]) {
            translate([sx * (cableR + cableKeywayOuterCornerRadius), mouthY - cableKeywayOuterCornerRadius])
                circle(r = cableKeywayOuterCornerRadius);
        }
    }
}

module fw100DeckNotchedProfile2d() {
    difference() {
        circle(d = deckOd);
        fw100CableKeywayCut2d();
    }
}

module fw100DeckNotched() {
    linear_extrude(height = deckThickness)
        fw100DeckNotchedProfile2d();
}

// -----------------------------------------------------------------------------
// Deck-MPU: prototype controller deck
// -----------------------------------------------------------------------------
// Two parallel slots are placed symmetrically around the center-to-edge
// reference line. The controller pins sit in these slots; a zip tie can retain
// the board for prototype testing.
mpuSlotOffsetFromCenterline = 11.43;
mpuSlotWidth = 3.00;
mpuSlotLengthFromEdge = 55.00;

module fw100RoundedSlot2d(len, width) {
    r = width / 2;
    hull() {
        translate([0, -len / 2 + r]) circle(r = r);
        translate([0,  len / 2 - r]) circle(r = r);
    }
}

module fw100DeckMpuSlots2d() {
    deckR = deckOd / 2;
    slotCenterY = deckR - (mpuSlotLengthFromEdge / 2);

    for (xoff = [-mpuSlotOffsetFromCenterline, mpuSlotOffsetFromCenterline]) {
        translate([xoff, slotCenterY])
            fw100RoundedSlot2d(mpuSlotLengthFromEdge, mpuSlotWidth);
    }
}

module fw100DeckMpuProfile2d() {
    difference() {
        circle(d = deckOd);
        fw100DeckMpuSlots2d();
    }
}

module fw100DeckMPU() {
    linear_extrude(height = deckThickness)
        fw100DeckMpuProfile2d();
}

// -----------------------------------------------------------------------------
// Layout preview
// -----------------------------------------------------------------------------
module fw100LayoutPreview() {
    spacing = 135;

    translate([0 * spacing, 0, 0]) fw100Body();
    translate([1 * spacing, 0, 0]) fw100TopCap();
    translate([2 * spacing, 0, 0]) fw100ConicalCap();
    translate([3 * spacing, 0, 0]) fw100RetainingNut();

    translate([0 * spacing, -spacing, 0]) fw100LoadSpreader();
    translate([1 * spacing, -spacing, 0]) fw100Retainer();
    translate([2 * spacing, -spacing, 0]) fw100DeckToF();
    translate([3 * spacing, -spacing, 0]) fw100DeckNotched();

    translate([0 * spacing, -2 * spacing, 0]) fw100DeckMPU();
}

// -----------------------------------------------------------------------------
// Render dispatch
// -----------------------------------------------------------------------------
if (part == "body") {
    fw100Body();
} else if (part == "cap_blank") {
    fw100TopCap();
} else if (part == "cap_conical") {
    fw100ConicalCap();
} else if (part == "nut") {
    fw100RetainingNut();
} else if (part == "load_spreader") {
    fw100LoadSpreader();
} else if (part == "retainer") {
    fw100Retainer();
} else if (part == "deck_tof") {
    fw100DeckToF();
} else if (part == "deck_notched") {
    fw100DeckNotched();
} else if (part == "deck_mpu") {
    fw100DeckMPU();
} else if (part == "layout") {
    fw100LayoutPreview();
} else {
    assert(false, str("Unknown FW100 part selector: ", part));
}

// Useful OpenSCAD console echoes.
echo(str("fitSlop = ", fitSlop));
echo(str("threadSlop = ", threadSlop));
echo(str("innerBoreId = ", innerBoreId));
echo(str("supportHoleId = ", supportHoleId));
echo(str("deckOd = ", deckOd));
echo(str("retainerOd = ", retainerOd));
echo(str("deck-tof slot = ", deckTofSlotY, " x ", deckTofSlotX, ", offset X = ", deckTofSlotOffsetX));
echo(str("deck-notched keyway dia = ", cableKeywayDia));
echo(str("deck-mpu slots offset = ", mpuSlotOffsetFromCenterline, ", length from edge = ", mpuSlotLengthFromEdge));
