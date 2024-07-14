

#ifndef STUDIOCONTROLLERACTIONS_H
#define STUDIOCONTROLLERACTIONS_H

#include <QWidget>

#include "AutomatableModel.h"

namespace lmms
{

class StudioControllerActions : public QWidget
{
	Q_OBJECT
public:
	StudioControllerActions();
	virtual ~StudioControllerActions();

	void setModels(
		FloatModel * homeModel,
		FloatModel * stopModel,
		FloatModel * playModel,
		FloatModel * recordModel,
		FloatModel * scrollModel,
		FloatModel * nextModel,
		FloatModel * prevModel
	);
	
signals:

public slots:
	void doHome();
	void doStop();
	void doPlay();
	void doNext();
	void doPrev();
	void doRecord();
	void doScroll();

private:
	float m_scrollLast;
	
	FloatModel * m_homeModel;
	FloatModel * m_stopModel;
	FloatModel * m_playModel;
	FloatModel * m_recordModel;
	FloatModel * m_scrollModel;
	FloatModel * m_nextModel;
	FloatModel * m_prevModel;
	
};

} // namespace lmms

#endif // STUDIOCONTROLLERACTIONS_H
