#ifndef LMMS_OSCILLATORBEZIER_H
#define LMMS_OSCILLATORBEZIER_H

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


class LMMS_EXPORT OscillatorBezier
{
	MM_OPERATORS
public:
	// TODO import points[n][4]
	OscillatorBezier( );
	virtual ~OscillatorBezier()
	{
	}

	sample_t oscSample(const float sample);
	sample_t bezierSample(float sample);

private:
	// number of bezier curves
	int m_number_of_segments;
	Point m_segments[4][4];
	// "t" reference bezeri math
	float m_last_t;
	// "i" index in tow hich bezier curve we use
	int m_last_i;
};

}
#endif // LMMS_OSCILLATORBEZIER_H
