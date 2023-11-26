#ifndef HALFSWING_H
#define HALFSWING_H

#include <QtCore/QObject>

#include "Groove.h"
#include "Knob.h"
#include "lmms_basics.h"
#include "Note.h"

namespace lmms
{


/**
 * A groove thatjust latter half of the HydrogenSwing algo.
 */
class HalfSwing : public QObject, public Groove
{
	Q_OBJECT
public:
	HalfSwing(QObject * parent );
	~HalfSwing() override;

	static QString name()
	{
		return "half";
	}

	void apply( Note * _n ) override;

	void loadSettings( const QDomElement & _this ) override;
	void saveSettings( QDomDocument & _doc, QDomElement & _element ) override;
	inline virtual QString nodeName() const override
	{
		return name();
	}

	QWidget * instantiateView( QWidget * _parent )  override;

	FloatModel m_swingAmountModel;

public slots:
	void updateAmount();

private:
	float m_swingFactor;// =  (m_swingAmount / 127.0)

} ;

}

namespace lmms::gui
{

class HalfSwingView : public QWidget
{
	Q_OBJECT
public:
	HalfSwingView(HalfSwing * _half_swing, QWidget * parent=0 );
	~HalfSwingView();

private:
	Knob * m_nob;

} ;

}

#endif // HALFSWING_H
