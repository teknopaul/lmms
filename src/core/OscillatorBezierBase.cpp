
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
	m_bisection_steps(DEFAULT_BISECTION_STEPS),
	m_last_t(0.0f),
	m_last_i(0),
	m_t_diff(0.0f)
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

// Solve x(t)=x_target on [0,1] using a few simple bisections.
// Assumes the segment is forward in x overall (P0.x <= P3.x).
static inline float solve_t_for_x(float x0, float x1, float x2, float x3, float last_t, float x_diff, float x_target,
								  int newton_steps, int bisection_steps)
{
	// Initial guess, t from last iteration, theoretically, t must be greater than x_last (or we have a square wave)
	// std::nextafter(x_last, 1.0f) is technically better guess
	float t = last_t;
	float lo = t;
	float hi = t + (x_diff * 10.0f); // this magic number comes from trial and error, annoying high freqs will hit 1.0f
	if (hi > 1.0f) hi = 1.0f;

	if (x_diff == 0.0f) {
		hi = 1.0f;
		bisection_steps += 4; // theoretical inital step of 0.0078125 * your curve
	}

	// guess x, then try again, only 4 steps seems OK, presumably because we have a pretty good hi and lo estimate
	// theoretically there are more preceise methods, in practice this works fine
	for (int i = 0; i < bisection_steps; ++i) {
		const float x = bezier_comp(x0, x1, x2, x3, t);
		// exit loop when we have a good x,  1e-6f is sample precise(ish)
		if (std::fabs(x - x_target) < 1e-6f) break;

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
		m_last_t = 0.0f;
		if (segment_index == 0) {
			// at a Z crossing
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
	const float t = solve_t_for_x(p0.x, p1.x, p2.x, p3.x, m_last_t, m_t_diff, sample, m_newton_steps, m_bisection_steps);

	// Return y(t) in [-1,1]
	const float y = bezier_comp(p0.y, p1.y, p2.y, p3.y, t);

	// save first t we calculate to use as a good estimate for hi
	if (sample > 1e-6f && m_t_diff == 0.0f) {
		m_t_diff = std::nextafter(t, 1.0f);
	}

	// save t because its the input to our next iteration as the best starting point for solve_t_for_x
	m_last_t = t;
	// save segment_index because when it changes is when we need to reset t
	m_last_i = segment_index;

	// tiny DC guard / clamp
	return clampf(y, -1.0f, 1.0f);

}

} // namespace lmms
