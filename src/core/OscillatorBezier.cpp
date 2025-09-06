
#include "OscillatorBezier.h"


namespace lmms
{

OscillatorBezier::OscillatorBezier()
{}

struct Point {
	float x, y;
};

// cubic Bézier evaluation (only y is used for oscillator output)
static inline float bezierY(const Point &p0, const Point &p1, const Point &p2, const Point &p3, float t)
{
	float u = 1.0f - t;
	return u*u*u*p0.y
		 + 3*u*u*t*p1.y
		 + 3*u*t*t*p2.y
		 + t*t*t*p3.y;
}

// Oscillator from SVG path drawn in Inkscape converted by ChatGPT5
// Normalized so that _sample ∈ [0,1) → y ∈ [-1,1]
// Already normalized (x → 0..1, y → -1..1).
// Each row is one cubic Bézier segment.
static const Point segments[4][4] = {
	{ {0.0000f, 0.954f}, {0.152f,  0.954f}, {0.193f, -0.012f}, {0.270f, -0.711f} },
	{ {0.270f, -0.711f}, {0.309f, -1.000f}, {0.385f, -0.997f}, {0.550f, -0.994f} },
	{ {0.550f, -0.994f}, {0.716f, -0.991f}, {0.765f, -1.063f}, {0.857f, -0.644f} },
	{ {0.857f, -0.644f}, {0.949f, -0.226f}, {0.902f,  0.954f}, {1.000f,  0.954f} }
};

// TODO
// invert the above points for side chain
// import from any svg
// Validate / force that...
//   first point x=0
//   last point x=1
//   x increases for each point
// bezier points dont go out of range (hard ish to calculate if any point will go out of range, could run it and see)
// could also limit range to 1.0 and =1.0 at runtime, that would cost more I guess, best do it once
// Starting with any Y is OK for a Control oscillator but not for sound which should start at 0,0 and end at 1,0
// Option to save bezier path to a  wavetable
// GUI to draw paths in LMMS

// Main oscillator function
sample_t OscillatorBezier::bezierSample(const float sample)
{
	const int numSegments = 4;
	float segPos = sample * numSegments;
	int segIndex = (int)segPos;
	if (segIndex >= numSegments) segIndex = numSegments - 1;
	float t = segPos - segIndex;  // local param in [0,1]

	const Point *p = segments[segIndex];
	return bezierY(p[0], p[1], p[2], p[3], t);
}

} // namespace lmms
