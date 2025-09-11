#include <cmath>
#include <algorithm>
#include <QDebug>
#include "OscillatorBezierZ.h"
#include "OscillatorBezierBase.h"


namespace lmms
{

// OscillatorBezierBase handles 2 to 4 curves (Point m_segments[4][4])

/**
 * 2 point bezier curves that draw a specific sound wave.
 * This is its SVG representaion, as draw in Inkscape.
 * <path
 *      style="fill:none;stroke:#000000;stroke-width:0.0060154px;stroke-linecap:butt;stroke-linejoin:miter;stroke-opacity:1"
 *      d="M 3.1669362e-4,0.5 C 0.24957661,7.5995137e-4 8.2341817e-4,0.99765707 0.5,0.5 0.99917658,0.00234293 0.75036395,1.0016561 1.0002507,0.5"
 *      id="path876" />
 * I have clamped the points to 1/4, it looks OK, does not produce square waves,
 * and because I think maybe mathematically coherant sound waves might be appealing
 * to the ear.
 * It is symetrical, so should have a DC offset of 0.0.
 */
OscillatorBezierZ::OscillatorBezierZ(float mod) :
	OscillatorBezierBase(),
	m_next_mod(-1.0f)
{
	overrideNumOfSegment(2);

	overrideSegments({
		// N.B starts at {0.0,0.0} ends at {1.0,0.0} so that wave starts and ends at zero amplitude
		// Despite being only 2 bezier curves, this produces 2 cycles over x=1 which is to say its apparent frequncey
		// will be one octave above, but each cycle is not identical, so it cant be optimized
		// in compressor terms there are two "hard knees" and two "soft knees" in the same wave form
		{ {0.000f, 0.000f}, {0.250f,  0.100f}, {0.000f, -1.000f}, {0.500f, 0.0f} },
		{ {0.500f, 0.000f}, {1.000f, -1.000f}, {0.750f, -1.000f}, {1.0f, 0.0f} },
		{ {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f} },
		{ {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f} }
	});
	if (mod >= 0.0 && mod <= 1.0 ) {
		modulate(mod);
		applyModulations();
	}
}

/**
 * @param mod value from 0.0f to 1.0f
 */
void OscillatorBezierZ::modulate(float mod)
{
	m_next_mod = mod;
}

/**
 * vary m_segments[0][1][0] from 0.250f  to 0.150f
 * vary m_segments[1][2][0] from 0.750f  to 0.850f
 * this will alter how hard the hard knee phase is
 * hopefully this will make it go from nasty to nice :)
 */
void OscillatorBezierZ::applyModulations()
{
	if (m_next_mod < 0.0f) return;

	float diff = m_next_mod / 5;
	m_segments[0][1].x = 0.250f - diff;
	m_segments[1][2].x = 0.750f + diff;
	m_next_mod = -1.0f;
}

} // namespace lmms
