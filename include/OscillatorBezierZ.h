#ifndef OSCILLATORBEZIERZ_H
#define OSCILLATORBEZIERZ_H

#include "OscillatorBezierBase.h"

namespace lmms
{

class OscillatorBezierZ : public OscillatorBezierBase
{
public:
	OscillatorBezierZ();
	virtual ~OscillatorBezierZ()
	{
	}

public slots:
	void modulate(float mod) override;

protected:
	void applyModulations() override;

private:
	float m_mod;
	float m_next_mod;
};

} // end namespace lmms

#endif // OSCILLATORBEZIERZ_H
