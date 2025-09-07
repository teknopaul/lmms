/*
 * ControllerRackView.cpp - view for song's controllers
 *
 * Copyright (c) 2008-2009 Paul Giblock <drfaygo/at/gmail.com>
 * Copyright (c) 2010-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QApplication>
#include <QPushButton>
#include <QScrollArea>
#include <QMessageBox>
#include <QVBoxLayout>

#include "Song.h"
#include "embed.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "ControllerRackView.h"
#include "ControllerView.h"
#include "LfoController.h"
#include "DuckingController.h"
#include "SubWindow.h"

namespace lmms::gui
{


ControllerRackView::ControllerRackView() :
	QWidget(),
	m_nextIndex(0)
{
	setWindowIcon( embed::getIconPixmap( "controller" ) );
	setWindowTitle( tr( "Controller Rack" ) );

	m_scrollArea = new QScrollArea( this );
	m_scrollArea->setPalette( QApplication::palette( m_scrollArea ) );
	m_scrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );

	auto scrollAreaWidget = new QWidget(m_scrollArea);
	m_scrollAreaLayout = new QVBoxLayout( scrollAreaWidget );
	m_scrollAreaLayout->addStretch();
	scrollAreaWidget->setLayout( m_scrollAreaLayout );

	m_scrollArea->setWidget( scrollAreaWidget );
	m_scrollArea->setWidgetResizable( true );

	m_addLfoButton = new QPushButton( this );
	m_addLfoButton->setText( tr( "Add LFO" ) );

	m_addDuckingButton = new QPushButton( this );
	m_addDuckingButton->setText( tr( "Add Ducking" ) );

	connect( m_addLfoButton, SIGNAL(clicked()),
			this, SLOT(addLfoController()));
	connect( m_addDuckingButton, SIGNAL(clicked()),
			this, SLOT(addDuckingController()));

	Song * song = Engine::getSong();
	connect( song, SIGNAL(controllerAdded(lmms::Controller*)), SLOT(onControllerAdded(lmms::Controller*)));
	connect( song, SIGNAL(controllerRemoved(lmms::Controller*)), SLOT(onControllerRemoved(lmms::Controller*)));

	auto layout = new QVBoxLayout();
	layout->addWidget( m_scrollArea );
	layout->addWidget( m_addLfoButton );
	layout->addWidget( m_addDuckingButton );
	this->setLayout( layout );

	QMdiSubWindow * subWin = getGUI()->mainWindow()->addWindowedWidget( this );

	// No maximize button
	Qt::WindowFlags flags = subWin->windowFlags();
	flags &= ~Qt::WindowMaximizeButtonHint;
	subWin->setWindowFlags( flags );
	
	subWin->setAttribute( Qt::WA_DeleteOnClose, false );
	subWin->move( 680, 310 );
	subWin->resize( 350, 200 );
	subWin->setFixedWidth( 350 );
	subWin->setMinimumHeight( 200 );
}




void ControllerRackView::saveSettings( QDomDocument & _doc,
							QDomElement & _this )
{
	MainWindow::saveWidgetState( this, _this );
}




void ControllerRackView::loadSettings( const QDomElement & _this )
{
	MainWindow::restoreWidgetState( this, _this );
}




void ControllerRackView::deleteController( ControllerView * _view )
{
	Controller * c = _view->getController();

	if( c->connectionCount() > 0 )
	{
		QMessageBox msgBox;
		msgBox.setIcon( QMessageBox::Question );
		msgBox.setWindowTitle( tr("Confirm Delete") );
		msgBox.setText( tr("Confirm delete? There are existing connection(s) "
				"associated with this controller. There is no way to undo.") );
		msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
		if( msgBox.exec() != QMessageBox::Ok )
		{
			return;
		}
	}

	Song * song = Engine::getSong();
	song->removeController( c );
}




void ControllerRackView::onControllerAdded( Controller * controller )
{
	QWidget * scrollAreaWidget = m_scrollArea->widget();

	auto controllerView = new ControllerView(controller, scrollAreaWidget);

	connect( controllerView, SIGNAL(deleteController(lmms::gui::ControllerView*)),
		 this, SLOT(deleteController(lmms::gui::ControllerView*)), Qt::QueuedConnection );

	m_controllerViews.append( controllerView );
	m_scrollAreaLayout->insertWidget( m_nextIndex, controllerView );

	++m_nextIndex;
}




void ControllerRackView::onControllerRemoved( Controller * removedController )
{
	ControllerView * viewOfRemovedController = 0;

	QVector<ControllerView *>::const_iterator end = m_controllerViews.end();
	for ( QVector<ControllerView *>::const_iterator it = m_controllerViews.begin(); it != end; ++it)
	{
		ControllerView *currentControllerView = *it;
		if ( currentControllerView->getController() == removedController )
		{
			viewOfRemovedController = currentControllerView;
			break;
		}
	}

	if (viewOfRemovedController )
	{
		m_controllerViews.erase( std::find( m_controllerViews.begin(),
					m_controllerViews.end(), viewOfRemovedController ) );

		delete viewOfRemovedController;
		--m_nextIndex;
	}
}




void ControllerRackView::addLfoController()
{
	Engine::getSong()->addController( new LfoController( Engine::getSong() ) );
	setFocus();
}

void ControllerRackView::addDuckingController()
{
	Engine::getSong()->addController( new DuckingController( Engine::getSong() ) );
	setFocus();
}



void ControllerRackView::closeEvent( QCloseEvent * _ce )
 {
	if( parentWidget() )
	{
		parentWidget()->hide();
	}
	else
	{
		hide();
	}
	_ce->ignore();
 }


} // namespace lmms::gui
