#ifndef OSCILLATORBEZIERZ_H
#define OSCILLATORBEZIERZ_H

#include "OscillatorBezierU.h"

namespace lmms
{

class OscillatorBezierZ : public OscillatorBezierU
{
public:
	OscillatorBezierZ();
	virtual ~OscillatorBezierZ()
	{
	}

	void modulate(float mod);
};

} // end namespace lmms

#endif // OSCILLATORBEZIERZ_H
