#ifndef HYDROGENSWING_H
#define HYDROGENSWING_H

#include <QtCore/QObject>

#include "Groove.h"
#include "Knob.h"
#include "lmms_basics.h"
#include "Note.h"

namespace lmms
{


/**
 * A groove that mimics Hydrogen drum machine's swing feature
 */
class HydrogenSwing : public Model, public Groove
{
	Q_OBJECT
public:
	HydrogenSwing( QObject * parent );
	~HydrogenSwing() override;

	static QString name()
	{
		return "hydrogen";
	}

	void apply( Note * _n ) override;

	void loadSettings( const QDomElement & _this ) override;
	void saveSettings( QDomDocument & _doc, QDomElement & _element ) override;
	inline virtual QString nodeName() const  override
	{
		return "hydrogen";
	}

	QWidget * instantiateView( QWidget * _parent )  override;

	FloatModel m_swingAmountModel;

public slots:
	void updateAmount();

private:
	float m_swingFactor;// =  (m_swingAmount / 127.0)

	friend class HydrogenSwingView;
};

}

namespace lmms::gui
{

class HydrogenSwingView : public QWidget
{
	Q_OBJECT
public:
	HydrogenSwingView(HydrogenSwing * swing, QWidget * parent );
	~HydrogenSwingView();

private:
	Knob * m_nob;

} ;

}
#endif // HYDROGENSWING_H
