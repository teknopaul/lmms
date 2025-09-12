#ifndef OSCILLATOR_BEZIER_SIN_H
#define OSCILLATOR_BEZIER_SIN_H

#include "OscillatorBezierBase.h"

namespace lmms
{

class OscillatorBezierSin : public OscillatorBezierBase
{
public:
	OscillatorBezierSin(float mod = 0.0f);
	virtual ~OscillatorBezierSin()
	{
	}

public slots:
	void modulate(float mod) override;

protected:
	void applyModulations() override;

private:
	// -1 for no change
	float m_next_mod;
};

} // end namespace lmms

#endif // OSCILLATOR_BEZIER_SIN_H
