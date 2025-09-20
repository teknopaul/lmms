#ifndef LMMS_OSCILLATOR_BEZIER_HH_RIDE_H
#define LMMS_OSCILLATOR_BEZIER_HH_RIDE_H

#include <cstring>

#include "Engine.h"
#include "lmms_constants.h"
#include "lmmsconfig.h"
#include "AudioEngine.h"
#include "OscillatorConstants.h"
#include "SampleBuffer.h"
#include "OscillatorBezier.h"
#include "OscillatorBezierBase.h"

namespace lmms
{

class LMMS_EXPORT OscillatorBezierHhRide : public OscillatorBezierBase
{
	MM_OPERATORS
public:
	OscillatorBezierHhRide( );
	virtual ~OscillatorBezierHhRide()
	{
	}

	void modulate(float mod) override {
		// noop for OscillatorBezierHhRide, todo we could modulate the speed of the return to full volume
	}
	void applyModulations() override {

	}

};

}
#endif // LMMS_OSCILLATOR_BEZIER_HH_RIDE_H
