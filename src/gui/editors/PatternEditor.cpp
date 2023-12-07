/*
 * PatternEditor.cpp - basic main-window for editing patterns
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - https://lmms.io
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

#include "PatternEditor.h"

#include <QAction>
#include <QToolButton>

#include "ClipView.h"
#include "ComboBox.h"
#include "DataFile.h"
#include "embed.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "PatternStore.h"
#include "PatternTrack.h"
#include "PianoRoll.h"
#include "Song.h"
#include "StringPairDrag.h"
#include "TrackView.h"

#include "MidiClip.h"


namespace lmms::gui
{


PatternEditor::PatternEditor(PatternStore* ps) :
	TrackContainerView(ps),
	m_ps(ps)
{
	setModel(ps);
}




void PatternEditor::addSteps()
{
	makeSteps( false );
}

void PatternEditor::cloneSteps()
{
	makeSteps( true );
}




void PatternEditor::removeSteps()
{
	const TrackContainer::TrackList& tl = model()->tracks();

	for (const auto& track : tl)
	{
		if (track->type() == Track::Type::Instrument)
		{
			auto p = static_cast<MidiClip*>(track->getClip(m_ps->currentPattern()));
			p->removeSteps();
		}
	}
}




void PatternEditor::addSampleTrack()
{
	(void) Track::create( Track::Type::Sample, model() );
}




void PatternEditor::addAutomationTrack()
{
	(void) Track::create( Track::Type::Automation, model() );
}




void PatternEditor::removeViewsForPattern(int pattern)
{
	for( TrackView* view : trackViews() )
	{
		view->getTrackContentWidget()->removeClipView(pattern);
	}
}



void PatternEditor::saveSettings(QDomDocument& doc, QDomElement& element)
{
	MainWindow::saveWidgetState( parentWidget(), element );
}

void PatternEditor::loadSettings(const QDomElement& element)
{
	MainWindow::restoreWidgetState(parentWidget(), element);
}




void PatternEditor::dropEvent(QDropEvent* de)
{
	QString type = StringPairDrag::decodeKey( de );
	QString value = StringPairDrag::decodeValue( de );

	if( type.left( 6 ) == "track_" )
	{
		DataFile dataFile( value.toUtf8() );
		Track * t = Track::create( dataFile.content().firstChild().toElement(), model() );

		// Ensure pattern clips exist
		bool hasValidPatternClips = false;
		if (t->getClips().size() == m_ps->numOfPatterns())
		{
			hasValidPatternClips = true;
			for (int i = 0; i < t->getClips().size(); ++i)
			{
				if (t->getClips()[i]->startPosition() != TimePos(i, 0))
				{
					hasValidPatternClips = false;
					break;
				}
			}
		}
		if (!hasValidPatternClips)
		{
			t->deleteClips();
			t->createClipsForPattern(m_ps->numOfPatterns() - 1);
		}
		m_ps->updateAfterTrackAdd();

		de->accept();
	}
	else
	{
		TrackContainerView::dropEvent( de );
	}
}




void PatternEditor::updatePosition()
{
	//realignTracks();
	emit positionChanged( m_currentPosition );
}




void PatternEditor::makeSteps( bool clone )
{
	const TrackContainer::TrackList& tl = model()->tracks();

	for (const auto& track : tl)
	{
		if (track->type() == Track::Type::Instrument)
		{
			auto p = static_cast<MidiClip*>(track->getClip(m_ps->currentPattern()));
			if( clone )
			{
				p->cloneSteps();
			} else
			{
				p->addSteps();
			}
		}
	}
}

// Creates a clone of the current pattern track with the same content, but no clips in the song editor
// TODO: Avoid repeated code from cloneTrack and clearTrack in TrackOperationsWidget somehow
void PatternEditor::cloneClip()
{
	// Get the current PatternTrack id
	auto ps = static_cast<PatternStore*>(model());
	const int currentPattern = ps->currentPattern();

	PatternTrack* pt = PatternTrack::findPatternTrack(currentPattern);

	if (pt)
	{
		// Clone the track
		Track* newTrack = pt->clone();
		ps->setCurrentPattern(static_cast<PatternTrack*>(newTrack)->patternIndex());

		// Track still have the clips which is undesirable in this case, clear the track
		newTrack->lock();
		newTrack->deleteClips();
		newTrack->unlock();
	}
}


void PatternEditor::quantizeNotes(QuantizeAction mode)
{

	const TrackContainer::TrackList& tl = model()->tracks();

	for (const auto& track : tl)
	{
		if (track->type() == Track::Type::Instrument)
		{
			auto midiClip = static_cast<MidiClip*>(track->getClip(m_ps->currentPattern()));
			if ( midiClip )
			{
				for ( Note * n : midiClip->notes() )
				{
					if ( n->length() == TimePos( 0 ) )
					{
						continue;
					}

					if (mode == QuantizeAction::HumanizeTiming)
					{
						humanizeTiming(n);
						continue;
					}
					if (mode == QuantizeAction::HumanizeVelocity)
					{
						humanizeVelocity(n);
						continue;
					}
					if (mode == QuantizeAction::HumanizeLength)
					{
						humanizeLength(n);
						continue;
					}
					if (mode == QuantizeAction::Groove)
					{
						quantizeGroove(n);
						continue;
					}
					if (mode == QuantizeAction::RemoveGroove)
					{
						removeGroove(n);
						continue;
					}

					Note copy(*n);
					midiClip->removeNote( n );
					midiClip->addNote(copy, false);
				}
			}
		}
	}

	Engine::getSong()->setModified();
}

void PatternEditor::humanizeTiming(Note * n)
{
	f_cnt_t offset =  (double)std::rand() / RAND_MAX * Engine::framesPerTick();
	n->setNoteOffset(n->getNoteOffset() + offset);
}


void PatternEditor::humanizeVelocity(Note * n)
{
	volume_t newVol = n->getVolume() - (std::rand() % 5);
	if (newVol < n->getVolume()) // volume_t is unsigned this is test for overflow
	{
		n->setVolume(newVol);
	}
}


void PatternEditor::humanizeLength(Note * n)
{
	TimePos length = n->length();
	tick_t newlen = length.getTicks() + (std::rand() % 6) - 2;
	if (newlen > 4) {
		length.setTicks(newlen);
		n->setLength(length);
	}
}


void PatternEditor::quantizeGroove(Note * n)
{
	Engine::getSong()->globalGroove()->apply(n);
}

void PatternEditor::removeGroove(Note * n)
{
	n->setNoteOffset(0);
}



PatternEditorWindow::PatternEditorWindow(PatternStore* ps) :
	Editor(false),
	m_editor(new PatternEditor(ps))
{

	setWindowIcon(embed::getIconPixmap("pattern_track_btn"));
	setWindowTitle(tr("Pattern Editor"));
	setCentralWidget(m_editor);

	setAcceptDrops(true);
	m_toolBar->setAcceptDrops(true);
	connect(m_toolBar, SIGNAL(dragEntered(QDragEnterEvent*)), m_editor, SLOT(dragEnterEvent(QDragEnterEvent*)));
	connect(m_toolBar, SIGNAL(dropped(QDropEvent*)), m_editor, SLOT(dropEvent(QDropEvent*)));

	// TODO: Use style sheet
	if (ConfigManager::inst()->value("ui", "compacttrackbuttons").toInt())
	{
		setMinimumWidth(TRACK_OP_WIDTH_COMPACT + DEFAULT_SETTINGS_WIDGET_WIDTH_COMPACT + 2 * ClipView::BORDER_WIDTH + 384);
	}
	else
	{
		setMinimumWidth(TRACK_OP_WIDTH + DEFAULT_SETTINGS_WIDGET_WIDTH + 2 * ClipView::BORDER_WIDTH + 384);
	}

	m_playAction->setToolTip(tr("Play/pause current pattern (Space)"));
	m_stopAction->setToolTip(tr("Stop playback of current pattern (Space)"));


	// Pattern selector
	DropToolBar* patternSelectionToolBar = addDropToolBarToTop(tr("Pattern selector"));

	m_patternComboBox = new ComboBox(m_toolBar);
	m_patternComboBox->setFixedSize(200, ComboBox::DEFAULT_HEIGHT);
	m_patternComboBox->setModel(&ps->m_patternComboBoxModel);

	patternSelectionToolBar->addWidget(m_patternComboBox);


	// Track actions
	DropToolBar *trackAndStepActionsToolBar = addDropToolBarToTop(tr("Track and step actions"));


	trackAndStepActionsToolBar->addAction(embed::getIconPixmap("add_pattern_track"), tr("New pattern"),
						Engine::getSong(), SLOT(addPatternTrack()));
	trackAndStepActionsToolBar->addAction(embed::getIconPixmap("clone_pattern_track_clip"), tr("Clone pattern"),
						m_editor, SLOT(cloneClip()));
	trackAndStepActionsToolBar->addAction(embed::getIconPixmap("add_sample_track"),	tr("Add sample-track"),
						m_editor, SLOT(addSampleTrack()));
	trackAndStepActionsToolBar->addAction(embed::getIconPixmap("add_automation"), tr("Add automation-track"),
						m_editor, SLOT(addAutomationTrack()));

	auto stretch = new QWidget(m_toolBar);
	stretch->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	trackAndStepActionsToolBar->addWidget(stretch);


	// Step actions
	trackAndStepActionsToolBar->addAction(embed::getIconPixmap("step_btn_remove"), tr("Remove steps"),
						m_editor, SLOT(removeSteps()));
	trackAndStepActionsToolBar->addAction(embed::getIconPixmap("step_btn_add"), tr("Add steps"),
						m_editor, SLOT(addSteps()));
	trackAndStepActionsToolBar->addAction( embed::getIconPixmap("step_btn_duplicate"), tr("Clone Steps"),
						m_editor, SLOT(cloneSteps()));

	connect(&ps->m_patternComboBoxModel, SIGNAL(dataChanged()),
			m_editor, SLOT(updatePosition()));

	auto viewNext = new QAction(this);
	connect(viewNext, SIGNAL(triggered()), m_patternComboBox, SLOT(selectNext()));
	viewNext->setShortcut(Qt::Key_Plus);
	addAction(viewNext);

	auto viewPrevious = new QAction(this);
	connect(viewPrevious, SIGNAL(triggered()), m_patternComboBox, SLOT(selectPrevious()));
	viewPrevious->setShortcut(Qt::Key_Minus);
	addAction(viewPrevious);

	DropToolBar *notesActionsToolBar = addDropToolBarToTop( tr( "Edit actions" ) );

	auto quantizeButton = new QToolButton(notesActionsToolBar);
	auto quantizeButtonMenu = new QMenu(quantizeButton);

	auto applyGrooveAction = new QAction(tr("Apply groove"), this);
	auto removeGrooveAction = new QAction(tr("Remove groove"), this);
	auto humanizeVelocityAction = new QAction(tr("Humanize velocity"), this);
	auto humanizeTimingAction = new QAction(tr("Humanize timing"), this);
	auto humanizeLengthAction = new QAction(tr("Humanize length"), this);

	connect(applyGrooveAction, &QAction::triggered, [this](){ m_editor->quantizeNotes(PatternEditor::QuantizeAction::Groove); });
	connect(removeGrooveAction, &QAction::triggered, [this](){ m_editor->quantizeNotes(PatternEditor::QuantizeAction::RemoveGroove); });
	connect(humanizeVelocityAction, &QAction::triggered, [this](){ m_editor->quantizeNotes(PatternEditor::QuantizeAction::HumanizeVelocity); });
	connect(humanizeTimingAction, &QAction::triggered, [this](){ m_editor->quantizeNotes(PatternEditor::QuantizeAction::HumanizeTiming); });
	connect(humanizeLengthAction, &QAction::triggered, [this](){ m_editor->quantizeNotes(PatternEditor::QuantizeAction::HumanizeLength); });

	applyGrooveAction->setShortcut( Qt::CTRL | Qt::Key_G );
	humanizeVelocityAction->setShortcut( Qt::CTRL | Qt::Key_H );

	quantizeButton->setPopupMode(QToolButton::MenuButtonPopup);
	quantizeButton->setDefaultAction(applyGrooveAction);
	quantizeButton->setMenu(quantizeButtonMenu);
	quantizeButtonMenu->addAction(applyGrooveAction);
	quantizeButtonMenu->addAction(removeGrooveAction);
	quantizeButtonMenu->addAction(humanizeVelocityAction);
	quantizeButtonMenu->addAction(humanizeTimingAction);
	quantizeButtonMenu->addAction(humanizeLengthAction);

	notesActionsToolBar->addSeparator();
	notesActionsToolBar->addWidget(quantizeButton);

}


QSize PatternEditorWindow::sizeHint() const
{
	return {minimumWidth() + 10, 300};
}


void PatternEditorWindow::play()
{
	if (Engine::getSong()->playMode() != Song::PlayMode::Pattern)
	{
		Engine::getSong()->playPattern();
	}
	else
	{
		Engine::getSong()->togglePause();
	}
}


void PatternEditorWindow::stop()
{
	Engine::getSong()->stop();
}


void PatternEditorWindow::stopAndGoBack()
{
	Engine::getSong()->stopAndGoBack(nullptr, Song::PlayMode::Pattern);
}

} // namespace lmms::gui
