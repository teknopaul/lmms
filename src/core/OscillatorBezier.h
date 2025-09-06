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

class LMMS_EXPORT OscillatorBezier
{
	MM_OPERATORS
public:
	// TODO import points[n][4]
	OscillatorBezier( );
	virtual ~OscillatorBezier()
	{
	}

	sample_t bezierSample(float sample);

};

}
#endif // LMMS_OSCILLATORBEZIER_H
