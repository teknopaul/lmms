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
	HalfSwing(QObject *parent=0 );

	virtual ~HalfSwing();

	void init();
	int amount();

	void apply( Note * _n ) override;

	void loadSettings( const QDomElement & _this ) override;
	void saveSettings( QDomDocument & _doc, QDomElement & _element ) override;
	inline virtual QString nodeName() const override
	{
		return "half";
	}

	QWidget * instantiateView( QWidget * _parent ) override;

signals:
	void swingAmountChanged(int _newAmount);


public slots:
	// valid values are from 0 - 127
	void setAmount(int _amount);
	void update();

private:
	int m_frames_per_tick;
	int m_swingAmount;
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

public slots:
	void modelChanged();
	void valueChanged(float);

private:
	HalfSwing * m_half_swing;
	FloatModel * m_nobModel;
	Knob * m_nob;

} ;

}

#endif // HALFSWING_H
