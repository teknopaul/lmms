
#include <cmath>
#include <algorithm>
#include <QDebug>
#include "OscillatorBezier.h"
#include "OscillatorBezierBase.h"


namespace lmms
{

/**
 * An oscillator that uses bezier math.
 * Bezier curve draws a smoothed shape, across a grid with x=1 and y from -1.0 to 1.0, i.e.; y is a wave function.
 * Goal is to ensure that the changes to volume are smooth.
 *
 * There are 2 to 6 bezier curves that make up the shape, number of curves does not affect performance.
 * There is hard plynominal math here that we trust because it works.
 * If the math is wrong its very convincing and produces functional output.
 *
 * This code is perf sensitive, it runs for almost every sample, wave table synth is faster.
 * Consider this a luxury for Ducking, I wrote it as a stepping stone to the bezier powered synth.
 */

// TODO
// import from any svg
// Validate / force that...
//   first point x=0
//   last point x=1
//   x increases for each point
// bezier points dont go out of range (hard-ish to calculate if _any_ point will go out of range, could run a it few times and see)
// Starting with any Y is OK for a Control oscillator but not for sound which should start at 0,0 and end at 1,0
// Option to save bezier path to a  wavetable
// GUI to draw paths in LMMS


OscillatorBezierBase::OscillatorBezierBase() :
	OscillatorBezier(),
	m_segments{
		{ {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f} },
		{ {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f} },
		{ {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f} },
		{ {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f} },
		{ {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f} },
		{ {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f} }
	},
	m_number_of_segments(MAX_BEZIER_SEGMENTS),
	m_newton_steps(DEFAULT_NEWTON_STEPS),
	m_bisection_steps(4),
	m_last_t(0),
	m_last_i(0)
{
	applyModulations();
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
static inline float solve_t_for_x(float x0, float x1, float x2, float x3, float x_last, float x_target,
								  int newton_steps, int bisection_steps)
{
	// Initial guess, t from last iteration, theoretically, t must be greater than x_last (or we have a square wave)
	// std::nextafter(x_last, 1.0f) is technically better, may not be necessary
	float t = std::nextafter(x_last, 1.0f);
	float lo = t;  // bounding on next after t prevents interations producing the same value
	float hi = 1.0f; // calculating a closer bound not worth it

	// Hybrid: a few Newton steps, clamped; if it stalls, use bisection refinement.
	for (int iter = 0; newton_steps < 4; ++iter) {
		const float x  = bezier_comp(x0, x1, x2, x3, t);
		const float dx = x - x_target;
		const float d1 = bezier_comp_d1(x0, x1, x2, x3, t);
		if (std::fabs(dx) < 1e-6f) break; // 1e-6f is idiomatic C++ for "very small sample", per ChatGPT, but is it in LMMS?

		// Maintain bracket
		if (dx > 0.0f) {
			hi = t;
		} else {
			lo = t;
		}

		if (std::fabs(d1) > 1e-9f) { // 1e-9f is idiomatic C++ for "very small float" (i.e. close to epsilon())
			float t_new = t - dx / d1;
			// If Newton left [lo,hi], do a bisection step instead
			if (t_new < lo || t_new > hi) {
				t = 0.5f * (lo + hi);
			}
			else {
				t = t_new;
			}
		} else {
			t = 0.5f * (lo + hi); // derivative too small; bisect
		}
		t = clampf(t, 0.0f, 1.0f);
	}

	// TODO experimentation to see if this is necessary after caching "t" fix
	// with accurate "t" perhaps even 4 newton steps is excessive?  Hence configurable
	// A couple of bisection cleanups for robustness
	for (int i = 0; i < bisection_steps; ++i) {
		const float x = bezier_comp(x0, x1, x2, x3, t);
		if (x > x_target) hi = t; else lo = t;
		t = 0.5f * (lo + hi);
	}

	// TODO debug mode with Peak detection, it would indicate a bug if we return a number >1.0 or <-1.0
	// Subclasses might be more risky than this class's m_segments
	// TODO clampf _should_ not be necessary here if we control input values, (and our math is solid)
	return clampf(t, 0.0f, 1.0f);
}

// as per wave-shape-routines in Oscillator.h
sample_t OscillatorBezierBase::oscSample(const float sample)
{
	float ph = absFraction( sample );
	return bezierSample( ph );
}

/**
 * Main oscillator function.
 * sample is the value passed by LMMS to all oscillators which is x between 0.0 and 1.0 see absFraction()
 * Behaviour outside 0 - 1.0 is undefined.
 */
sample_t OscillatorBezierBase::bezierSample(const float sample)
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
		if (segment_index == 0) {
			applyModulations();
		}
	}
	// TODO support for single segment curves by detecting sample = 0.0 and resetting t
	// do we get 0.0 or sample < 1e-6f? setting "t" to zero for low sample values is probably OK,
	// "t" accidentally high is probably not OK.
	// maybe sample < m_last_sample, but that requries state specific to single segment oscillators
	// maybe create OscillatorBezier1 which takes one curve only, and cache sample instead of last_i

	// t is “how far along this particular bezier segment we are.” its not simply x value so we need to calculate it by guessing and iterating
	// fortunatly for audio its very close to the t from last time we ran this function, so our starting guess is very good.

	const Point& p0 = m_segments[segment_index][0];
	const Point& p1 = m_segments[segment_index][1];
	const Point& p2 = m_segments[segment_index][2];
	const Point& p3 = m_segments[segment_index][3];

	// Solve x(t) = s for this segment
	const float t = solve_t_for_x(p0.x, p1.x, p2.x, p3.x, m_last_t, sample, m_newton_steps, m_bisection_steps);

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
