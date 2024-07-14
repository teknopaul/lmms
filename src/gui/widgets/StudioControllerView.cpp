/*
 * StudioControllerView.cpp allows using a mido controller for functions such as play, pause, stop and a jog wheel 
 *
 * Copyright (c) 2017 teknopaul <teknopaul/at/teknopaul.com>
 *
 * This file is part of LMMS - http://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */


#include <QApplication>
#include <QInputDialog>
#include <QLayout>
#include <QPushButton>
#include <QComboBox>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QMdiArea>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QMdiSubWindow>

#include <QtXml/QDomElement>

#include "embed.h"
#include "AutomatableModelView.h"
#include "Engine.h"
#include "InstrumentTrack.h"
#include "MainWindow.h"
#include "MidiClient.h"
#include "MidiClip.h"
#include "Mixer.h"
#include "MixerView.h"
#include "PianoRoll.h"
#include "PianoView.h"
#include "Song.h"
#include "SongEditor.h"
#include "SubWindow.h"
#include "GuiApplication.h"
#include "StudioControllerView.h"
#include "ControllerConnection.h"
#include "MidiController.h"

//
// TODO this is both view and model, data is stored in QDropDown and QLabels
//

namespace lmms::gui
{


StudioControllerView::StudioControllerView() :
	QWidget(),
	m_lastAutowiredPort(NULL)
{
	setMinimumWidth( 250 );
	setMinimumHeight( 410 );
	resize( 250, 540 );

	setWindowIcon( embed::getIconPixmap( "controller" ) );
	setWindowTitle( tr( "Studio Controller" ) );

	m_layout = new QVBoxLayout();
	this->setLayout( m_layout );

	SubWindow * subWin = getGUI()->mainWindow()->addWindowedWidget( this );

	// No maximize button
	Qt::WindowFlags flags = subWin->windowFlags();
	flags &= ~Qt::WindowMaximizeButtonHint;
	subWin->setWindowFlags(flags);

	parentWidget()->setAttribute( Qt::WA_DeleteOnClose, false );
	parentWidget()->move( 90, 90 );


	m_keyboardLabel = new QLabel("Default Keyboard");
	m_layout->addWidget( m_keyboardLabel );

	m_defaultKeyboardDropDown = new QComboBox(this);
	m_defaultKeyboardDropDown->setInsertPolicy(QComboBox::InsertAtBottom);
	m_layout->addWidget( m_defaultKeyboardDropDown );

	m_defaultKeyboard = new QLabel("");
	m_layout->addWidget( m_defaultKeyboard );

	m_layout->addSpacing(15);

	m_controllerLabel = new QLabel("Studio Controller");
	m_layout->addWidget( m_controllerLabel );

	m_controllerFileDropDown = new QComboBox(this);
	m_controllerFileDropDown->setInsertPolicy(QComboBox::InsertAtBottom);
	m_controllerFileDropDown->insertItem(0, "No Studio Controller" , QVariant::fromValue(0) );
	m_controllerFileDropDown->insertSeparator(1);
	m_controllerFileDropDown->setCurrentIndex(0);
	m_layout->addWidget( m_controllerFileDropDown );

	m_matchedController = new QLabel("");
	m_layout->addWidget( m_matchedController );

	m_saveButton = new QPushButton("save", this);
	m_layout->addWidget( m_saveButton );

	m_saveasButton = new QPushButton("save as", this);
	m_layout->addWidget( m_saveasButton );

	m_autoWireFxFxButton = new QPushButton("wire fx", this);
	m_layout->addWidget( m_autoWireFxFxButton );

	m_layout->addSpacing(15);

	// LMMS midi configurable actions
	m_homeButton = new AutomatableControlButton(this, "home");
	m_homeButton->setText("home");
	m_layout->addWidget( m_homeButton );

	m_stopButton = new AutomatableControlButton(this, "stop");
	m_stopButton->setText("stop");
	m_layout->addWidget( m_stopButton );

	m_playButton = new AutomatableControlButton(this, "play");
	m_playButton->setText("play");
	m_layout->addWidget( m_playButton );

	m_recordButton = new AutomatableControlButton(this, "record");
	m_recordButton->setText("record");
	m_layout->addWidget( m_recordButton );

	m_scrollButton = new AutomatableControlButton(this, "scroll");
	m_scrollButton->setText("scroll");
	m_layout->addWidget( m_scrollButton );

	m_nextButton = new AutomatableControlButton(this, "next");
	m_nextButton->setText("next");
	m_layout->addWidget( m_nextButton );

	m_prevButton = new AutomatableControlButton(this, "prev");
	m_prevButton->setText("prev");
	m_layout->addWidget( m_prevButton );

	m_layout->addStretch();

	m_actions = new StudioControllerActions();
	m_actions->setModels(
		m_homeButton->model(),
		m_stopButton->model(),
		m_playButton->model(),
		m_recordButton->model(),
		m_scrollButton->model(),
		m_nextButton->model(),
		m_prevButton->model()
	);

	/*
	connect( m_homeButton->model(),   SIGNAL( dataChanged() ), m_actions, SLOT( doHome() ) );
	connect( m_stopButton->model(),   SIGNAL( dataChanged() ), m_actions, SLOT( doStop() ) );
	connect( m_playButton->model(),   SIGNAL( dataChanged() ), m_actions, SLOT( doPlay() ) );
	connect( m_recordButton->model(), SIGNAL( dataChanged() ), m_actions, SLOT( doRecord() ) );
	connect( m_scrollButton->model(), SIGNAL( dataChanged() ), m_actions, SLOT( doScroll() ) );
	connect( m_nextButton->model(),   SIGNAL( dataChanged() ), m_actions, SLOT( doNext() ) );
	connect( m_prevButton->model(),   SIGNAL( dataChanged() ), m_actions, SLOT( doPrev() ) );
	*/

	// controller actions
	connect( m_saveButton, SIGNAL( clicked() ), this, SLOT( overwriteStudioController() ) );
	connect( m_saveasButton, SIGNAL( clicked() ), this, SLOT( saveStudioController() ) );
	connect( m_autoWireFxFxButton, SIGNAL( clicked() ), this, SLOT( autoWireFx() ) );
	connect( m_controllerFileDropDown, SIGNAL( activated(int) ), this, SLOT( controllerFileChanged(int) ) );
	connect( m_defaultKeyboardDropDown, SIGNAL( activated(int) ), this, SLOT( defaultKeyboardChanged(int) ) );

	Engine::audioEngine()->midiClient()->connectRPChanged( this, SLOT( listMidiControllers() ) );

	connect( getGUI()->pianoRoll(), SIGNAL( currentMidiClipChanged() ), this, SLOT( autoWireKeyboard() ) );

	listMidiControllers();
	listStudioControllerFiles();

	matchKeyboard(ConfigManager::inst()->value( "midi", "keyboard" ));
	m_controllerFileDropDown->setCurrentIndex(m_controllerFileDropDown->findText(ConfigManager::inst()->value( "midi", "controller")));
	loadStudioController();

	update();
}

StudioControllerView::~StudioControllerView()
{
	delete m_actions;

	delete m_keyboardLabel;
	delete m_defaultKeyboardDropDown;
	delete m_defaultKeyboard;
	delete m_controllerLabel;
	delete m_controllerFileDropDown;
	delete m_matchedController;
	delete m_layout;

	delete m_saveButton;
	delete m_saveasButton;
	delete m_autoWireFxFxButton;

	delete m_homeButton;
	delete m_stopButton;
	delete m_playButton;
	delete m_recordButton;
	delete m_scrollButton;
	delete m_nextButton;
	delete m_prevButton;
}


QString StudioControllerView::getDefaultKeyboard()
{
	QString curr = m_defaultKeyboard->text();
	if ("No Keyboard" == curr) return NULL;
	return curr;
}

// Match up attached controllers to the file selected.
// i.e. you cant use a korg config for some other controller hardware
// "20:0 nanoKONTROL Studio:nanoKONTROL Studio MIDI 1" Ignore leading numbers since this changes when you plug and unplug devices
void StudioControllerView::matchController(const QString& name)
{

	QString found = matchMidiDevice(name);

	if (found != "")
	{
		m_matchedController->setText(found);
		loadStudioController();
	}
	else
	{
		m_matchedController->setText("");
	}

}

void StudioControllerView::matchKeyboard(const QString& name)
{
	QString found =  matchMidiDevice(name);

	if (found != "")
	{
		m_defaultKeyboard->setText(found);
		m_defaultKeyboardDropDown->setCurrentIndex(m_defaultKeyboardDropDown->findText(found));
	}
	else
	{
		m_defaultKeyboard->setText("No Keyboard");
		m_defaultKeyboardDropDown->setCurrentIndex(0);
	}

}

// "20:0 nanoKONTROL Studio:nanoKONTROL Studio MIDI 1" Ignore leading numbers since this changes when you plug and unplug devices
QString StudioControllerView::matchMidiDevice(const QString& name)
{
	// QT 5 has something liekthis
	// Engine::mixer()->midiClient()->readablePorts().filter(/[0-9]:[0-9] name:.*/)

	int start = name.indexOf(" ", 0);
	if (++start == 0)
	{
		qWarning("Midicontroller name not as expected, no initial space %s", name .toUtf8().data());
		static QString empty;
		return empty;
	}

	int len = name.indexOf(":", start) - start;
	QString toMatch = name.mid(start, len);

	QStringList readablePorts = Engine::audioEngine()->midiClient()->readablePorts();
	for (QStringList::iterator it = readablePorts.begin(); it != readablePorts.end(); ++it )
	{
		QString current = *it;

		start = current.indexOf(" ", 0);
		if (++start == 0) continue;

		len = current.indexOf(":", start) - start;
		if (current.mid(start, len) == toMatch)
		{
			return current;
		}
	}

	static QString empty;
	return empty;

}
/**
 * List attached Midi Controllers and update list
 */
void StudioControllerView::listMidiControllers()
{

	m_defaultKeyboardDropDown->clear();
	m_defaultKeyboardDropDown->insertItem(0, "No Keyboard" , QVariant::fromValue(0) );
	m_defaultKeyboardDropDown->insertSeparator(1);
	m_defaultKeyboardDropDown->setCurrentIndex(0);

	QStringList readablePorts = Engine::audioEngine()->midiClient()->readablePorts();
	for (QStringList::iterator it = readablePorts.begin(); it != readablePorts.end(); ++it )
	{
		QString current = *it;
		m_defaultKeyboardDropDown->addItem(current , QVariant::fromValue(0) );
	}

}

/**
 * List all controller XML files.
 */
void StudioControllerView::listStudioControllerFiles()
{
	QString homePath = QDir::homePath();
	QDir dir(homePath + "/lmms/controllers/");
	QStringList nameFilters;
	nameFilters << "*.controller.xml";
	QFileInfoList allFiles = dir.entryInfoList(nameFilters, QDir::NoDotAndDotDot | QDir::Files);

	for (QFileInfoList::iterator it = allFiles.begin(); it != allFiles.end(); ++it )
	{
		QFileInfo current = *it;
		m_controllerFileDropDown->addItem(current.baseName ()  , QVariant::fromValue(0) );
	}
}
/**
 * Read controller XML file to find the Midi Controller name
 */
void StudioControllerView::loadStudioControllerName()
{

	if (m_controllerFileDropDown->currentText() == "No Studio Controller")
	{
		unloadStudioController();
	}
	else
	{
		QString homePath = QDir::homePath();
		QString fileName(homePath + "/lmms/controllers/" + m_controllerFileDropDown->currentText() + ".controller.xml");

		QFile inFile( fileName );
		if ( ! inFile.open(QIODevice::ReadOnly | QIODevice::Text) )
		{
			QMessageBox::critical( NULL, "Could not open file", "Could not open file" + fileName);
			return;
		}

		QDomDocument doc;
		if ( ! doc.setContent(&inFile) )
		{
			inFile.close();
			qWarning("could not read XML %s", fileName.toUtf8().data());
			return;
		}
		inFile.close();

		QDomElement root = doc.documentElement();

		QDomNodeList midiControllerList = doc.elementsByTagName("Midicontroller");
		if (midiControllerList.size() > 0) {

			QDomElement midiControllerElem = midiControllerList.item(0).toElement();
			matchController(midiControllerElem.attribute("inports"));

		}
		else
		{
			qWarning("No Midicontrollers found in the XML file");
		}
	}
}

void StudioControllerView::controllerFileChanged(int index)
{
	loadStudioControllerName();

	ConfigManager::inst()->setValue( "midi", "controller", m_controllerFileDropDown->currentText());
	ConfigManager::inst()->saveConfigFile();
}


void StudioControllerView::defaultKeyboardChanged(int index)
{
	// TODO save default piano, make it available for autowiring piano roll
	QString keyb = m_defaultKeyboardDropDown->currentText();

	m_defaultKeyboard->setText(keyb);

	ConfigManager::inst()->setValue( "midi", "keyboard", keyb);
	ConfigManager::inst()->saveConfigFile();
}

/**
 * Write controller config to a DOM, N.B. does not save this widgets data to the song.
 */
QDomElement saveSettings(QDomDocument & doc, gui::AutomatableControlButton* button)
{
	QDomElement elem = doc.createElement( button->text() );
	AutomatableModel* amp = dynamic_cast<AutomatableModel*>(button->model());
	if (amp) amp->saveSettings(doc, elem, button->text() );
	return elem;
}

void loadSettings(const QDomElement & elem, gui::AutomatableControlButton* button)
{

	AutomatableModel* amp = dynamic_cast<AutomatableModel*>(button->model());
	if (amp) {
		amp->loadSettings(elem, button->text() );
	}
	else
	{
		qWarning("no config for %s", button->text().toUtf8().data());
	}

}

/**
 * Save MidiController to LMMS action mappings to a file.
 */
void StudioControllerView::saveStudioController()
{
	bool ok;
	QString name = QInputDialog::getText( this, "Controller name", "Enter controller name", QLineEdit::Normal, "myStudioController", &ok);
	name = name.trimmed();

	if (ok && ! name.isEmpty())
	{
		m_controllerFileDropDown->addItem(name , QVariant::fromValue(0) );
		m_controllerFileDropDown->setCurrentIndex(m_controllerFileDropDown->findText(name));

		overwriteStudioController(true);
	}
}

void StudioControllerView::overwriteStudioController()
{
	overwriteStudioController(false);
}

/**
 * Save MidiController to LMMS action mappings to a file.
 */
void StudioControllerView::overwriteStudioController(bool force)
{
	if ( force || QMessageBox::question( this, "Overwrite?", "Overwrite controller information?", QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes)
	{


		QDomDocument doc = QDomDocument( "lmms-studio-controller" );
		QDomElement root = doc.createElement( "lmms-studio-controller" );
		root.setAttribute( "creator", "LMMS" );
		doc.appendChild(root);
		root.appendChild(saveSettings(doc, m_homeButton));
		root.appendChild(saveSettings(doc, m_nextButton));
		root.appendChild(saveSettings(doc, m_playButton));
		root.appendChild(saveSettings(doc, m_prevButton));
		root.appendChild(saveSettings(doc, m_recordButton));
		root.appendChild(saveSettings(doc, m_scrollButton));
		root.appendChild(saveSettings(doc, m_stopButton));

		QString homePath = QDir::homePath();
		QDir dir = QDir::root();
		dir.mkdir( homePath + "/lmms/controllers" );

		QString fileName(homePath + "/lmms/controllers/" + m_controllerFileDropDown->currentText() + ".controller.xml");

		QFile outfile( fileName );
		if (! outfile.open(QIODevice::WriteOnly | QIODevice::Text) )
		{
			QMessageBox::critical( NULL, "Could not save file", "Could not save file" + fileName);
		}
		else
		{
			QTextStream ts( &outfile );
			ts << doc.toString();
			outfile.close();
		}
		qWarning("saved %s", fileName.toUtf8().data());

	}
}


void StudioControllerView::unloadStudioController()
{
	// TODO
	m_homeButton->removeConnection();
	m_stopButton->removeConnection();
	m_playButton->removeConnection();
	m_recordButton->removeConnection();
	m_scrollButton->removeConnection();
	m_nextButton->removeConnection();
	m_prevButton->removeConnection();

	m_matchedController->setText("");
}

void StudioControllerView::loadStudioController()
{

	if ( m_matchedController->text() == "" )
	{
		return;
	}

	QString homePath = QDir::homePath();
	QString fileName(homePath + "/lmms/controllers/" + m_controllerFileDropDown->currentText() + ".controller.xml");

	QFile inFile( fileName );
	if ( ! inFile.open(QIODevice::ReadOnly | QIODevice::Text) )
	{
		QMessageBox::critical( NULL, "Could not open file", "Could not open file" + fileName);
		return;
	}

	QDomDocument doc;
	if ( ! doc.setContent(&inFile) )
	{
		inFile.close();
		qWarning("could not read XML %s", fileName.toUtf8().data());
		return;
	}
	inFile.close();

	QDomElement root = doc.documentElement();

	// Fiddle the Midicontroller inports to the currently loaded midid device
	QDomNodeList midiControllerList = doc.elementsByTagName("Midicontroller");
	for (int m = 0 ; m < midiControllerList.size() ; m++)
	{

		midiControllerList.item(m).toElement().setAttribute("inports", m_matchedController->text());

	}

	QDomNodeList buttonList = root.childNodes();
	for (int i = 0 ; i < buttonList.size() ; i++)
	{

		if (buttonList.item(i).nodeName() == "home" )   loadSettings(buttonList.item(i).toElement(), m_homeButton);
		if (buttonList.item(i).nodeName() == "stop" )   loadSettings(buttonList.item(i).toElement(), m_stopButton);
		if (buttonList.item(i).nodeName() == "play" )   loadSettings(buttonList.item(i).toElement(), m_playButton);
		if (buttonList.item(i).nodeName() == "record" ) loadSettings(buttonList.item(i).toElement(), m_recordButton);
		if (buttonList.item(i).nodeName() == "scroll" ) loadSettings(buttonList.item(i).toElement(), m_scrollButton);
		if (buttonList.item(i).nodeName() == "next" )   loadSettings(buttonList.item(i).toElement(), m_nextButton);
		if (buttonList.item(i).nodeName() == "prev" )   loadSettings(buttonList.item(i).toElement(), m_prevButton);

	}

}

/**
 * If there is a default keyboard, when piano-roll changes wire up the keyboard to the instrument.
 * Provided it is not already wired up
 */
void StudioControllerView::autoWireKeyboard()
{
	QString keyb = getDefaultKeyboard();
	if (keyb == NULL) return;

	const MidiClip * clip = getGUI()->pianoRoll()->currentMidiClip();
	if (clip)
	{
		InstrumentTrack * t = clip->instrumentTrack();
		if (t)
		{
			MidiPort * mp = t->midiPort();
			if (mp)
			{
				if ( ! mp->isInputEnabled() )
				{
					if (m_lastAutowiredPort) {
						// TODO we might not always want to unwire here
						// we need to be sure that we setEnabled before unenabling
						m_lastAutowiredPort->setReadable(false);
					}
					mp->setReadable(true);
					mp->subscribeReadablePort( keyb, true );
					m_lastAutowiredPort = mp;
				}
			}
		}

	}
}

/**
 * Wire up sliders to any existing FxChannels, that are not automated or already wired up.
 */
void StudioControllerView::autoWireFx()
{

	QString homePath = QDir::homePath();
	QString fileName(homePath + "/lmms/controllers/" + m_controllerFileDropDown->currentText() + ".controller.xml");

	QFile inFile( fileName );
	if ( ! inFile.open(QIODevice::ReadOnly | QIODevice::Text) )
	{
		QMessageBox::critical( NULL, "Could not open file", "Could not open file" + fileName);
		return;
	}

	QDomDocument doc;
	if ( ! doc.setContent(&inFile) )
	{
		inFile.close();
		qWarning("could not read XML %s", fileName.toUtf8().data());
		return;
	}
	inFile.close();

	QDomElement root = doc.documentElement();

	QDomNodeList buttonList = root.childNodes();
	for (int i = 0 ; i < buttonList.size() ; i++)
	{

		if (buttonList.item(i).nodeName() == "fx-autowire" )
		{

			QDomElement autowireElem = buttonList.item(i).toElement();
			QDomNodeList autowireChannelList = autowireElem.childNodes();


			for (i = 0 ; i < autowireChannelList.size() && i < Engine::mixer()->numChannels() ; i++) {

				 auto mixerChannelView = getGUI()->mixerView()->channelView(i);

				 if (mixerChannelView)
				 {

					Fader * fader = mixerChannelView->m_fader;
					FloatModel* floatModel = dynamic_cast<FloatModel*>(fader->model());
					// this does not work either
					AutomatableModel* amp = dynamic_cast<AutomatableModel*>(fader->model());
					if (floatModel && amp && ! amp->isAutomatedOrControlled() )
					{
						float savedVal = floatModel->value(); // Read correctly between 0.0 and 2.

						QDomElement elem = autowireChannelList.item(i).toElement();

						// this works but screws the current volume in the process
						floatModel->loadSettings(elem, "volume");

						// these don't work why?? I can set the vol to zero but not set it back
						// The MidiController just created has a model, set that!
						floatModel->setValue( savedVal );
						floatModel->setAutomatedValue( savedVal );
						floatModel->setInitValue( savedVal );

						ControllerConnection* cc = amp->controllerConnection();
						if (cc) {
							Controller* midiController = cc->getController();
							if (midiController) {
								FloatModel* controllerModel = dynamic_cast<FloatModel*>(midiController);
								if (controllerModel)
								{
									controllerModel->setValue(savedVal);
								}
							}
						}
						qWarning("wired fx channel %d, %f", i, savedVal);
					}


				}
				else
				{
					break;
				}
			}
			break;
		}

	}
	getGUI()->mixerView()->refreshDisplay();
}

} // namespace lmms::gui
