

#ifndef STUDIOCONTROLLERVIEW_H
#define STUDIOCONTROLLERVIEW_H


#include <QLabel>
#include <QWidget>
#include <QCloseEvent>

#include <QComboBox>
#include <QVBoxLayout>

#include "MidiPort.h"
#include "SerializingObject.h"
#include "AutomatableControlButton.h"
#include "StudioControllerActions.h"

namespace lmms::gui
{


class StudioControllerView : public QWidget
{
	Q_OBJECT
public:
	StudioControllerView();
	virtual ~StudioControllerView();
	
	QString getDefaultKeyboard();


public slots:
	void controllerFileChanged(int index);
	void defaultKeyboardChanged(int index);

	void saveStudioController();
	void loadStudioController();
	void unloadStudioController();
	void overwriteStudioController();
	void autoWireFx();
	void autoWireKeyboard();
	void listMidiControllers();

	
private:
	StudioControllerActions * m_actions;
	
	QLabel * m_keyboardLabel;
	QComboBox * m_defaultKeyboardDropDown;
	QLabel * m_defaultKeyboard;
	QLabel * m_controllerLabel;
	QComboBox * m_controllerFileDropDown;
	QLabel * m_matchedController;
	QVBoxLayout * m_layout;
	
	QPushButton* m_saveButton;
	QPushButton* m_saveasButton;
	QPushButton* m_autoWireFxFxButton;

	AutomatableControlButton  * m_homeButton;
	AutomatableControlButton  * m_stopButton;
	AutomatableControlButton  * m_playButton;
	AutomatableControlButton  * m_recordButton;
	AutomatableControlButton  * m_scrollButton;
	AutomatableControlButton  * m_nextButton;
	AutomatableControlButton  * m_prevButton;
	
	MidiPort * m_lastAutowiredPort;
	
	void loadStudioControllerName();
	QString matchMidiDevice(const QString& name);
	void matchKeyboard(const QString& name);
	void matchController(const QString& name);
	void listStudioControllerFiles();
	void overwriteStudioController(bool force);

	
};

} // namespace lmms::gui

#endif // STUDIOCONTROLLERVIEW_H
