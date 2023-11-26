#ifndef GROOVEEXPERIMENTS_H
#define GROOVEEXPERIMENTS_H

#include <QtCore/QObject>

#include "Groove.h"
#include "Knob.h"
#include "lmms_basics.h"
#include "Note.h"

namespace lmms
{

	
/**
 * A groove thats new
 */
class GrooveExperiments : public QObject, public Groove
{
	Q_OBJECT
public:
	GrooveExperiments( QObject * parent );
	~GrooveExperiments() override;

	static QString name()
	{
		return "experiment";
	}

	void updateAmount();

	void apply( Note * _n ) override;

	void loadSettings( const QDomElement & _this ) override;
	void saveSettings( QDomDocument & _doc, QDomElement & _element ) override;
	inline virtual QString nodeName() const  override
	{
		return name();
	}

	QWidget * instantiateView( QWidget * _parent )  override;

	FloatModel m_swingAmountModel;

private:
	float m_swingFactor;// =  (m_shiftAmount / 127.0)

};

}

namespace lmms::gui
{

class GrooveExperimentsView : public QWidget
{
	Q_OBJECT
public:
	GrooveExperimentsView(GrooveExperiments * swing, QWidget * parent );
	~GrooveExperimentsView();

private:
	Knob * m_nob;

};

}

#endif // GROOVEEXPERIMENTS_H
