#ifndef OSCILLATOR_BEZIER_USER_H
#define OSCILLATOR_BEZIER_USER_H

#include "OscillatorBezierBase.h"

namespace lmms
{

class OscillatorBezierUser;

const int MAX_MODULATIONS = 6;

struct ModulationDef {
	float range;
	float start;
	// pointer to the number that will change created from this
	int segment;
	int item;
	bool x; // or y
};

struct Modulation {
	float range;
	float start;
	// pointer to the number that will change
	float * coord;
};

/**
 * The definitions of the class, a clone is created for each notePlay()
 */
class OscillatorBezierDefinition
{
public:
	OscillatorBezierDefinition();
	virtual ~OscillatorBezierDefinition()
	{

	}
	QString m_svgFile;
	int m_number_of_segments;
	Point m_segments[MAX_BEZIER_SEGMENTS][BEZIER_POINTS];
	int m_mod_count;
	ModulationDef m_modulation_defs[MAX_MODULATIONS];

	void setModulations(OscillatorBezierUser * osc);

	QString openSvgFile();
	QString getFile();
	int loadFromSVG(QString path);

private:
	bool parseBezierPath(const QString &d);
};

class OscillatorBezierUser : public OscillatorBezierBase
{
public:
	OscillatorBezierUser(OscillatorBezierDefinition * oscDef);
	virtual ~OscillatorBezierUser()
	{
	}


	void modulate(float mod) override;
	void applyModulations() override;

private:
	void loadModulations(OscillatorBezierDefinition * oscDef);

	Modulation m_modulations[MAX_MODULATIONS];
	int m_mod_count;
	float m_last_mod;
	float m_mod;
};

} // end namespace lmms

#endif // OSCILLATOR_BEZIER_USER_H
