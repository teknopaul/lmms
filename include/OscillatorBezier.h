#ifndef LMMS_OSCILLATOR_BEZIER_H
#define LMMS_OSCILLATOR_BEZIER_H

#include <cstring>

#include "Engine.h"
#include "lmms_constants.h"
#include "lmmsconfig.h"
#include "AudioEngine.h"
#include "OscillatorConstants.h"

namespace lmms
{


struct Point {
	float x, y;
};


class LMMS_EXPORT OscillatorBezier : public QObject
{
	Q_OBJECT
public:
	virtual ~OscillatorBezier() = default;
	/**
	 * returns the y position for the given sample
	 * sample from -1.0 to 1.0 
	 */
	virtual sample_t oscSample(const float sample) = 0;
	/**
	 * modulate the curve in some way, need not be implemented.
	 * This is called whenever a user/lfo/automation changes the mutate knob,
	 * it should take effect only in the next cycle when x and y are 0.0 to avoid clicks.
	 * mod range is from 0.0f to 1.0f
	 */
	virtual void modulate(float mod) = 0;

	/**
	 * This method should apply mod to the bezier vectors(s)
	 * It will be callsed when x is zero.
	 */
	virtual void applyModulations() = 0;

};

} // namespace lmms

#endif // LMMS_OSCILLATOR_BEZIER_H
