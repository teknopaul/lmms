#ifndef LMMS_OSCILLATOR_BEZIER_BASE_H
#define LMMS_OSCILLATOR_BEZIER_BASE_H

#include <cstring>

#include "Engine.h"
#include "lmms_constants.h"
#include "lmmsconfig.h"
#include "AudioEngine.h"
#include "OscillatorConstants.h"
#include "SampleBuffer.h"
#include "OscillatorBezier.h"

namespace lmms
{

const int MAX_BEZIER_SEGMENTS = 6;
const int BEZIER_POINTS = 4;
const int DEFAULT_NEWTON_STEPS = 4;

class LMMS_EXPORT OscillatorBezierBase : public OscillatorBezier
{
	MM_OPERATORS
public:
	OscillatorBezierBase( );
	virtual ~OscillatorBezierBase()
	{
	}

	// precision / perf tradeoff
	inline void setNewtonStep( float newton_steps )
	{
		m_newton_steps = newton_steps;
	}

	sample_t oscSample(const float sample) override;

	void modulate(float mod) override {
		// noop for BezierBase
	}
	void applyModulations() override {
		// noop for BezierBase
	}

protected:
	Point m_segments[MAX_BEZIER_SEGMENTS][BEZIER_POINTS];

	void overrideNumOfSegment(int num_of_segments) {
		m_number_of_segments = num_of_segments;
	}
	void overrideSegments(const Point (&init)[MAX_BEZIER_SEGMENTS][BEZIER_POINTS]) {
		std::memcpy(m_segments, init, sizeof(m_segments));
	}


private:
	// number of bezier curves (2 to 6)
	int m_number_of_segments;
	// number of steps (precision) of sample to curve calculation
	// TODO try out some values since last_t optimization maybe 1 is sufficient
	int m_newton_steps;
	int m_bisection_steps;
	// "t" reference bezier alog
	float m_last_t;
	// "i" index in to which bezier curve we use
	int m_last_i;

	sample_t bezierSample(float sample);
};

}
#endif // LMMS_OSCILLATOR_BEZIER_BASE_H
