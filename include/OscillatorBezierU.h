#ifndef LMMS_OSCILLATOR_BEZIER_U_H
#define LMMS_OSCILLATOR_BEZIER_U_H

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

class LMMS_EXPORT OscillatorBezierU : public OscillatorBezierBase
{
	MM_OPERATORS
public:
	OscillatorBezierU( );
	virtual ~OscillatorBezierU()
	{
	}

	void modulate(float mod) override {
		// noop for BezierU, todo we could modulate the speed of the return to full volume
	}
	void applyModulations() override {

	}

};

}
#endif // LMMS_OSCILLATOR_BEZIER_U_H
