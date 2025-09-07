#ifndef LMMS_OSCILLATORBEZIERU_H
#define LMMS_OSCILLATORBEZIERU_H

#include <cstring>

#include "Engine.h"
#include "lmms_constants.h"
#include "lmmsconfig.h"
#include "AudioEngine.h"
#include "OscillatorConstants.h"
#include "SampleBuffer.h"

namespace lmms
{


struct Point {
	float x, y;
};


class LMMS_EXPORT OscillatorBezierU
{
	MM_OPERATORS
public:
	// TODO import points[n][4]
	OscillatorBezierU( );
	virtual ~OscillatorBezierU()
	{
	}

	// precision / perf tradeoff
	inline void setNewtonStep( float newton_steps )
	{
		m_newton_steps = newton_steps;
	}

	sample_t oscSample(const float sample);
	sample_t bezierSample(float sample);

protected:
	void overrideNumOfSegment(int num_of_segments) {
		m_number_of_segments = num_of_segments;
	}
	void overrideSegments(const Point (&init)[4][4]) {
		std::memcpy(m_segments, init, sizeof(m_segments));
	}


private:
	// number of bezier curves
	int m_number_of_segments;
	// number of steps (precision) of sample to curve calculation
	// TODO try out some values since last_t optimization maybe 1 is sufficient
	int m_newton_steps;
	int m_bisection_steps;
	Point m_segments[4][4];
	// "t" reference bezeri math
	float m_last_t;
	// "i" index in to which bezier curve we use
	int m_last_i;
};

}
#endif // LMMS_OSCILLATORBEZIERU_H
