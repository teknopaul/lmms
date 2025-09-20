
#include <cmath>
#include <algorithm>
#include <QDebug>
#include "OscillatorBezierBase.h"
#include "OscillatorBezierU.h"


namespace lmms
{

/**
 * A ducking oscillator that uses bezier math.
 * Bezier curve draws a smoothed very specific U shape, across a grid with x=1 and y from -1.0 to 1.0, i.e.; y is a wave function.
 * Goal is to ensure that the changes to volume are smooth. (square wave sucks for any ducking)
 *   The volume as the kick plays is lowest. (typically you can phase adjust in DuckingController)
 *   Return to full volume is fast (sine wave sucks for ducking kicks).
 *
 * The points on the bezeir curves we trust literally because it looks nice visually and conceptually.
 * The curve was drawn in Inkscape.
 *
 * This code is perf sensitive, it runs for almost every sample, wave table synth is faster.
 * Consider this a luxury for Ducking, I wrote it as a stepping stone to a bezier powered synth.
 *
 * Not useful for an audio wave: starts at +-1.0 and has an ugly DC offset.
 */
OscillatorBezierU::OscillatorBezierU() : OscillatorBezierBase()
{
	overrideNumOfSegment(4);

	overrideSegments({
		 // N.B starts at 0,-1 ends at 1,-1
		 { {0.000f, -0.950f}, {0.152f, 0.950f}, {0.193f, 0.080f}, {0.270f, 0.711f} },
		 { {0.270f,  0.711f}, {0.309f, 1.000f}, {0.385f, 1.000f}, {0.550f, 1.000f} },
		 { {0.550f,  1.000f}, {0.716f, 1.000f}, {0.765f, 1.000f}, {0.857f, 0.644f} },
		 { {0.857f,  0.644f}, {0.949f, 0.226f}, {0.900f, -0.95f}, {1.000f,  -0.95f} },
		 { {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f} },
		 { {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f} }
	});
}

} // namespace lmms
