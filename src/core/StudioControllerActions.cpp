/*
 * StudioControllerActions.cpp Controller functions such as play, pause, stop and a jog wheel.
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
#include <QDebug>

#include "StudioControllerActions.h"

#include "embed.h"
#include "Engine.h"
#include "MainWindow.h"
#include "Song.h"
#include "SongEditor.h"
#include "GuiApplication.h"
#include "ControllerConnection.h"
#include "MidiController.h"
#include "AutomatableModel.h"
#include "MidiClient.h"
#include "PianoRoll.h"


namespace lmms
{

StudioControllerActions::StudioControllerActions()
{
	
	m_scrollLast = 0.0f;
	
}

StudioControllerActions::~StudioControllerActions()
{

}

void StudioControllerActions::setModels(
		FloatModel * homeModel,
		FloatModel * stopModel,
		FloatModel * playModel,
		FloatModel * recordModel,
		FloatModel * scrollModel,
		FloatModel * nextModel,
		FloatModel * prevModel) {
	
	m_homeModel   = homeModel;
	m_stopModel   = stopModel;
	m_playModel   = playModel;
	m_recordModel = recordModel;
	m_scrollModel = scrollModel;
	m_nextModel   = nextModel;
	m_prevModel   = prevModel;
	
	connect( m_homeModel,   SIGNAL( dataChanged() ), this, SLOT( doHome() ) );
	connect( m_stopModel,   SIGNAL( dataChanged() ), this, SLOT( doStop() ) );
	connect( m_playModel,   SIGNAL( dataChanged() ), this, SLOT( doPlay() ) );
	connect( m_recordModel, SIGNAL( dataChanged() ), this, SLOT( doRecord() ) );
	connect( m_scrollModel, SIGNAL( dataChanged() ), this, SLOT( doScroll() ) );
	connect( m_nextModel,   SIGNAL( dataChanged() ), this, SLOT( doNext() ) );
	connect( m_prevModel,   SIGNAL( dataChanged() ), this, SLOT( doPrev() ) );

}


void StudioControllerActions::doHome()
{
	// Korg
	// Button down we get  0.496094
	// Button up we get 126.503906
	// TODO perhpas other controllers don't do control change like this?
	bool click = m_homeModel->value() < 1.0f;
	
	if (click) gui::getGUI()->songEditor()->stopAndGoBack();
	
}


void StudioControllerActions::doStop()
{
	
	bool click = m_stopModel->value() < 1.0f;
	
	if (click) gui::getGUI()->songEditor()->stop();
	
}


void StudioControllerActions::doPlay()
{
	
	bool click = m_playModel->value() < 1.0f;
	
	if (click) gui::getGUI()->songEditor()->play();
	
}


void StudioControllerActions::doNext()
{
	
	bool click = m_nextModel->value() < 1.0f;

	if (click) gui::getGUI()->songEditor()->next();

}


void StudioControllerActions::doPrev()
{
	
	bool click = m_prevModel->value() < 1.0f;
	
	if (click) gui::getGUI()->songEditor()->prev();

}


void StudioControllerActions::doRecord()
{
	
	bool click = m_recordModel->value() < 1.0f;
	
	if ( click && gui::getGUI()->pianoRoll()->isVisible() )
	{
		
		gui::getGUI()->pianoRoll()->recordAccompany();
		
	}

}


void StudioControllerActions::doScroll()
{
	
	float pos = m_scrollModel->value();
	
	if (pos > m_scrollLast ) gui::getGUI()->songEditor()->next();
	if (pos < m_scrollLast ) gui::getGUI()->songEditor()->prev();
	
	m_scrollLast = pos;
	
}

} // namespace lmms
