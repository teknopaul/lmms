
#include <cmath>
#include <algorithm>
#include <QDebug>
#include "OscillatorBezierBase.h"
#include "OscillatorBezierHhRide.h"


namespace lmms
{

/**
 * A ducking oscillator that uses bezier math.
 * Bezier curve draws a smoothed very specific shape for riding hh lines, across a grid with x=1 and y from -1.0 to 1.0, i.e.; y is a wave function.
 *
 * The points on the bezeir curves we trust literally because it looks nice visually and conceptually.
 * The curve was drawn in Inkscape.
 *
 * Not useful for an audio wave.
 */
OscillatorBezierHhRide::OscillatorBezierHhRide() : OscillatorBezierBase()
{
	overrideNumOfSegment(2);

	overrideSegments({
		 // N.B starts at 0,0 ends at 1,0
		 { {0.000f, 0.000f},  {0.300f,  0.000f}, {0.400f, -0.600f}, {0.600f, -0.600f} },
		 { {0.600f, -0.600f}, {0.750f, -0.600f}, {0.750f, 0.000f},  {1.000f, 0.000f} },
		 { {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f} },
		 { {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f} },
		 { {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f} },
		 { {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f} }
	});
}

} // namespace lmms
