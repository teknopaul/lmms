
#include <cmath>
#include <algorithm>
#include <QDebug>
#include "OscillatorBezierBase.h"
#include "OscillatorBezierV.h"


namespace lmms
{

/**
 * V Duck aka phatu. i.e. a more extreme duck, for bigger kicks.
 * A ducking oscillator that uses bezier math.
 * Bezier curve draws a more pointy very specific V shape, 
 * across a grid with x=1 and y from -1.0 to 1.0, i.e.; y is a wave function.
 * Lots of space for the kick and less for what follows.
 */
OscillatorBezierV::OscillatorBezierV() : OscillatorBezierBase()
{
	overrideNumOfSegment(2);

	overrideSegments({
		 // N.B starts at 0,1 ends at 1,1, we need to invert output phase for ducking
		 // TODO fix this grid so that we dont have * -1.0 in Base
		 { {0.00f, -0.95f}, {0.40f,  0.954f}, {0.45f, 1.0f}, {0.80f, 1.0f} },
		 { {0.800f, 1.00f}, {1.000f, 1.00f}, {0.85f, -0.95f}, {1.0f, -0.95f} },
		 { {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f} },
		 { {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f} },
		 { {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f} },
		 { {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f} }
	});
}

} // namespace lmms
