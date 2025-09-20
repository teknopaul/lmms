#include <cmath>
#include <algorithm>
#include <QDebug>
#include "OscillatorBezierSin.h"
#include "OscillatorBezierBase.h"


namespace lmms
{

// OscillatorBezierBase handles 2 to 4 curves (Point m_segments[4][4])

/**
 * 2 point bezier curves that draw a sin-like sound wave, 
 * and supports mutation to triangle-like wave.
 * Sin to Tri,  from nice to nasty
 */
OscillatorBezierSin::OscillatorBezierSin(float mod) :
	OscillatorBezierBase(),
	m_next_mod(-1.0f)
{
	overrideNumOfSegment(2);

	overrideSegments({
		// N.B starts at {0.0,0.0} ends at {1.0,0.0} so that wave starts and ends at zero amplitude
		//  this is a nice smooth sin type wave that mutates to nasty sinsaw
		{ {0.000f, 0.000f}, {0.020f,  -0.100f}, {0.450f, -0.100f}, {0.500f, 0.500f} },
		{ {0.500f, 0.500f}, {0.5500f,  0.100f}, {0.980f,  1.000f}, {1.000f, 0.000f} },
		{ {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f} },
		{ {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f} }
	});
	if (mod >= 0.0f && mod <= 1.0f ) {
		modulate(mod);
		applyModulations();
	}
}

/**
 * @param mod value from 0.0f to 1.0f
 */
void OscillatorBezierSin::modulate(float mod)
{
	m_next_mod = mod;
}

/**
 * vary m_segments[0][1][0] from 0.020f  to 0.000f
 * vary m_segments[1][2][0] from 0.980f  to 1.000f
 * this will make it go from smoothsounding sin to a nastZ wave:)
 * when mod is 1.0 you have a square wave, so that is inhibited
 */
void OscillatorBezierSin::applyModulations()
{
	if (m_next_mod < 0.0f) return;
	
	// inhibit square
	if (m_next_mod > 0.98f) m_next_mod = 0.98f;

	float diff = m_next_mod / 2;
	m_segments[0][1].x = 0.020f + diff;
	m_segments[1][2].x = 0.980f - diff;
	m_next_mod = -1.0f;
}

} // namespace lmms
