
#include <cmath>
#include <algorithm>
#include <QDebug>
#include "OscillatorBezier.h"


namespace lmms
{

/**
 * A ducking oscillator that use bezier math.
 * Bezeri curve draws a smothed very specific U shape, across a grid with x=1 and y from -1.0 to 1.0, i.e.; y is a wave function.
 * Goal is to ensure that the changes to volume are smooth.
 * The volume as the kick plays is lowest. (typically you can phase adjust in DuckingController)
 * Return to full volume is faster than a sine wave.
 * There are 4 bezier curves that make up the U shape, number of curves does not affect performance.
 * There is hard plynominal math here that we trust because it works.
 * If the math is wrong its very convincing and produces functional output.
 * The points on the bezeir curves we trust literally becuse it looks nice visually.
 * The curve was draw in Inkscape
 */

// TODO
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


OscillatorBezier::OscillatorBezier() :
	m_number_of_segments(4),
	m_segments{
		// N.B starts at 0,1 ends at 1,1, but for some reason we need to invert output phase for ducking
		{ {0.0000f, 0.954f}, {0.152f,  0.954f}, {0.193f, -0.012f}, {0.270f, -0.711f} },
		{ {0.270f, -0.711f}, {0.309f, -1.000f}, {0.385f, -0.997f}, {0.550f, -0.994f} },
		{ {0.550f, -0.994f}, {0.716f, -0.991f}, {0.765f, -1.063f}, {0.857f, -0.644f} },
		{ {0.857f, -0.644f}, {0.949f, -0.226f}, {0.902f,  0.954f}, {1.000f,  0.954f} }
	},
	m_last_t(0),
	m_last_i(0)
{
}

static inline float clampf(float v, float a, float b) {
	return v < a ? a : (v > b ? b : v);
}

// Evaluate cubic Bezier for x(t) or y(t).
// param t is how far along the curve we are (its not x)
static inline float bezier_comp(float p0, float p1, float p2, float p3, float t)
{
	const float u  = 1.0f - t;
	const float uu = u * u;
	const float tt = t * t;
	return (uu * u) * p0 + 3.0f * (uu * t) * p1 + 3.0f * (u * tt) * p2 + (tt * t) * p3;
}

// Derivative (for Newton step)
static inline float bezier_comp_d1(const float p0, const float p1, const float p2, const float p3, const float t)
{
	const float u = 1.0f - t;
	// 3*( (1-t)^2*(p1-p0) + 2*(1-t)*t*(p2-p1) + t^2*(p3-p2) )
	return 3.0f * (u*u * (p1 - p0) + 2.0f * u * t * (p2 - p1) + t*t * (p3 - p2));
}

// Solve x(t)=x_target on [0,1] using a few Newton iterations with bisection fallback.
// Assumes the segment is forward in x overall (P0.x <= P3.x).
static inline float solve_t_for_x(float x0, float x1, float x2, float x3, float x_last, float x_target)
{
	// Initial guess, x from last iteration
	float t = x_last;
	float lo = 0.0f, hi = 1.0f;

	// Hybrid: a few Newton steps, clamped; if it stalls, use bisection refinement.
	for (int iter = 0; iter < 4; ++iter) {
		const float x  = bezier_comp(x0, x1, x2, x3, t);
		const float dx = x - x_target;
		const float d1 = bezier_comp_d1(x0, x1, x2, x3, t);
		if (std::fabs(dx) < 1e-6f) break;

		// Maintain bracket
		if (dx > 0.0f) hi = t; else lo = t;

		if (std::fabs(d1) > 1e-9f) {
			float t_new = t - dx / d1;
			// If Newton left [lo,hi], do a bisection step instead
			if (t_new < lo || t_new > hi) t = 0.5f * (lo + hi);
			else t = t_new;
		} else {
			t = 0.5f * (lo + hi); // derivative too small; bisect
		}
		t = clampf(t, 0.0f, 1.0f);
	}

	// A couple of bisection cleanups for robustness
	for (int i = 0; i < 4; ++i) {
		const float x = bezier_comp(x0, x1, x2, x3, t);
		if (x > x_target) hi = t; else lo = t;
		t = 0.5f * (lo + hi);
	}
	return clampf(t, 0.0f, 1.0f);
}

// as per wave-shape-routines in Oscillator.h
sample_t OscillatorBezier::oscSample(const float sample)
{
	float ph = absFraction( sample );
	return bezierSample( ph );
}

/**
 * Main oscillator function,
 * sample is that value passed by LMMS to all oscilators which is between 0.0 and 1.0 see absFraction()
 */
sample_t OscillatorBezier::bezierSample(const float sample)
{
	// find segment the sample is inside
	int segment_index;
	for (segment_index = 0; segment_index < m_number_of_segments ; segment_index++) {
		if (sample <= m_segments[segment_index][3].x) {
			break;
		}
	}
	// when we change segments
	if (m_last_i != segment_index) {
		m_last_t = 0;
	}

	// t is “how far along this particular bezier segment we are.” its not simply x value so we need to calculate it by guessing and iterating
	// fortunatly for audio its very close to the t from last time we ran this function, so our starting guess is very good.

	const Point& p0 = m_segments[segment_index][0];
	const Point& p1 = m_segments[segment_index][1];
	const Point& p2 = m_segments[segment_index][2];
	const Point& p3 = m_segments[segment_index][3];

	// Solve x(t) = s for this segment
	const float t = solve_t_for_x(p0.x, p1.x, p2.x, p3.x, m_last_t, sample);

	// Return y(t) in [-1,1]
	const float y = bezier_comp(p0.y, p1.y, p2.y, p3.y, t);

	// save t because its the input to our next iteration as the best starting point for solve_t_for_x
	m_last_t = t;
	// save segment_index because when it changes is when we need to reset t
	m_last_i = segment_index;

	// tiny DC guard / clamp + invert phase
	return -1.0f * clampf(y, -1.0f, 1.0f);

}

} // namespace lmms
