#ifndef HYDROGENSWING_H
#define HYDROGENSWING_H

#include <QtCore/QObject>

#include "Groove.h"
#include "Knob.h"
#include "lmms_basics.h"
#include "Note.h"

namespace lmms {

/**
 * A groove that mimics Hydrogen drum machine's swing feature
 */
class HydrogenSwing : public QObject, public Groove
{
	Q_OBJECT
public:
	HydrogenSwing(QObject *parent=0 );

	virtual ~HydrogenSwing();

	void init();
	int amount();

	void apply( Note * _n ) override;

	void loadSettings( const QDomElement & _this ) override;
	void saveSettings( QDomDocument & _doc, QDomElement & _element ) override;
	inline virtual QString nodeName() const  override
	{
		return "hydrogen";
	}

	QWidget * instantiateView( QWidget * _parent )  override;

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

class HydrogenSwingView : public QWidget
{
	Q_OBJECT
public:
	HydrogenSwingView(HydrogenSwing * _hy_swing, QWidget * parent=0 );
	~HydrogenSwingView();

public slots:
	void modelChanged();
	void valueChanged(float);

private:
	HydrogenSwing * m_hy_swing;
	FloatModel * m_nobModel;
	Knob * m_nob;

} ;

}
#endif // HYDROGENSWING_H
