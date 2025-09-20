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
		for (int m = 0;  m < m_mod_count; m++) {
			delete m_modulation_defs[m];
		}
	}
	QString m_svgFile;
	int m_number_of_segments;
	Point m_segments[MAX_BEZIER_SEGMENTS][BEZIER_POINTS];
	int m_mod_count;
	ModulationDef * m_modulation_defs[MAX_MODULATIONS];
	QString m_name;

	void setModulations(OscillatorBezierUser * osc);

	QString openSvgFile();
	QString getFile();
	int loadFromSVG(QString path);

	QString getName()
	{
		return m_name;
	}

private:
	bool parseBezierPath(const QString &d);
	bool parseModulations(const QString &desc);
};

class OscillatorBezierUser : public OscillatorBezierBase
{
public:
	OscillatorBezierUser(OscillatorBezierDefinition * oscDef, float mod);
	virtual ~OscillatorBezierUser()
	{
	}


	void modulate(float mod) override;
	void applyModulations() override;

private:
	void initModulations(OscillatorBezierDefinition * oscDef);

	Modulation m_modulations[MAX_MODULATIONS];
	int m_mod_count;
	float m_next_mod;
};

} // end namespace lmms

#endif // OSCILLATOR_BEZIER_USER_H
