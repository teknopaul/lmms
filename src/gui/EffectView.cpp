/*
 * EffectView.cpp - view-component for an effect
 *
 * Copyright (c) 2006-2007 Danny McRae <khjklujn/at/users.sourceforge.net>
 * Copyright (c) 2007-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QGraphicsOpacityEffect>
#include <QLayout>
#include <QMetaEnum>
#include <QMouseEvent>
#include <QPushButton>
#include <QPainter>

#include "EffectView.h"
#include "DummyEffect.h"
#include "CaptionMenu.h"
#include "embed.h"
#include "FileDialog.h"
#include "GuiApplication.h"
#include "gui_templates.h"
#include "Knob.h"
#include "LedCheckBox.h"
#include "MainWindow.h"
#include "SubWindow.h"
#include "TempoSyncKnob.h"


namespace lmms::gui
{

EffectView::EffectView( Effect * _model, QWidget * _parent ) :
	PluginView( _model, _parent ),
	m_bg( embed::getIconPixmap( "effect_plugin" ) ),
	m_subWindow( nullptr ),
	m_controlView(nullptr),
	m_dragging(false)
{
	setFixedSize(EffectView::DEFAULT_WIDTH, EffectView::DEFAULT_HEIGHT);

	// Disable effects that are of type "DummyEffect"
	bool isEnabled = !dynamic_cast<DummyEffect *>( effect() );
	m_bypass = new LedCheckBox( this, "", isEnabled ? LedCheckBox::LedColor::Green : LedCheckBox::LedColor::Red );
	m_bypass->move( 3, 3 );
	m_bypass->setEnabled( isEnabled );

	m_bypass->setToolTip(tr("On/Off"));


	m_wetDry = new Knob( KnobType::Bright26, this );
	m_wetDry->setLabel( tr( "W/D" ) );
	m_wetDry->move( 40 - m_wetDry->width() / 2, 5 );
	m_wetDry->setEnabled( isEnabled );
	m_wetDry->setHintText( tr( "Wet Level:" ), "" );


	m_autoQuit = new TempoSyncKnob( KnobType::Bright26, this );
	m_autoQuit->setLabel( tr( "DECAY" ) );
	m_autoQuit->move( 78 - m_autoQuit->width() / 2, 5 );
	m_autoQuit->setEnabled( isEnabled && !effect()->m_autoQuitDisabled );
	m_autoQuit->setHintText( tr( "Time:" ), "ms" );


	m_gate = new Knob( KnobType::Bright26, this );
	m_gate->setLabel( tr( "GATE" ) );
	m_gate->move( 116 - m_gate->width() / 2, 5 );
	m_gate->setEnabled( isEnabled && !effect()->m_autoQuitDisabled );
	m_gate->setHintText( tr( "Gate:" ), "" );


	setModel( _model );

	if( effect()->controls()->controlCount() > 0 )
	{
		auto ctls_btn = new QPushButton(tr("Controls"), this);
		QFont f = ctls_btn->font();
		ctls_btn->setFont( pointSize<8>( f ) );
		ctls_btn->setGeometry( 150, 5, 60, 20 );
		connect( ctls_btn, SIGNAL(clicked()),
					this, SLOT(editControls()));

		m_controlView = effect()->controls()->createView();
		if( m_controlView )
		{
			m_subWindow = getGUI()->mainWindow()->addWindowedWidget( m_controlView );

			if ( !m_controlView->isResizable() )
			{
				m_subWindow->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
				if (m_subWindow->layout())
				{
					m_subWindow->layout()->setSizeConstraint(QLayout::SetFixedSize);
				}
			}

			Qt::WindowFlags flags = m_subWindow->windowFlags();
			flags &= ~Qt::WindowMaximizeButtonHint;
			m_subWindow->setWindowFlags( flags );

			connect( m_controlView, SIGNAL(closed()),
					this, SLOT(closeEffects()));

			m_subWindow->hide();
		}

		auto saveBtn = new QPushButton(tr("🖫"), this);
		f = saveBtn->font();
		saveBtn->setFont( pointSize<10>( f ) );
		saveBtn->setGeometry( 170, 34, 18, 18 );
		connect( saveBtn, SIGNAL(clicked()), this, SLOT(saveFxPreset()));
		auto loadBtn = new QPushButton(tr("..."), this);
		f = loadBtn->font();
		loadBtn->setFont( pointSize<9>( f ) );
		loadBtn->setGeometry( 192, 34, 18, 18 );
		connect( loadBtn, SIGNAL(clicked()), this, SLOT(loadFxPreset()));
	}
	
	m_opacityEffect = new QGraphicsOpacityEffect(this);
	m_opacityEffect->setOpacity(1);
	setGraphicsEffect(m_opacityEffect);

	//move above vst effect view creation
	//setModel( _model );
}




EffectView::~EffectView()
{
	delete m_subWindow;
}




void EffectView::editControls()
{
	if( m_subWindow )
	{
		if( !m_subWindow->isVisible() )
		{
			m_subWindow->show();
			m_subWindow->raise();
			effect()->controls()->setViewVisible( true );
		}
		else
		{
			m_subWindow->hide();
			effect()->controls()->setViewVisible( false );
		}
	}
}




void EffectView::moveUp()
{
	emit moveUp( this );
}




void EffectView::moveDown()
{
	emit moveDown( this );
}



void EffectView::deletePlugin()
{
	emit deletePlugin( this );
}




void EffectView::closeEffects()
{
	if( m_subWindow )
	{
		m_subWindow->hide();
	}
	effect()->controls()->setViewVisible( false );
}



void EffectView::saveFxPreset()
{
	FileDialog fd(this, tr("Save preset"), "", tr("FX preset (*.lfxp)"));

	fd.setAcceptMode(FileDialog::AcceptSave);
	fd.setFileMode(FileDialog::AnyFile);
	fd.setNameFilter("FX presets (*.lfxp)");
	fd.setDefaultSuffix("lfxp");
	fd.setDirectory(ConfigManager::inst()->factoryPresetsDir());

	if ( fd.exec() == QDialog::Accepted &&
		!fd.selectedFiles().isEmpty() &&
		!fd.selectedFiles().first().isEmpty() )
	{
		QFile fxFile(fd.selectedFiles()[0]);
		if (fxFile.open(QFile::WriteOnly))
		{
			Effect * efx = effect();
			QDomDocument doc("lmms-lfxp-file");

			QDomElement lfxp = doc.createElement("lfxp");
			doc.appendChild(lfxp);
			lfxp.setAttribute("version", efx->descriptor()->version);
			lfxp.setAttribute("name", efx->descriptor()->name);
			lfxp.setAttribute("type", QVariant::fromValue(efx->descriptor()->type).toString());
			effect()->saveSettings(doc, lfxp);
			QString data = doc.toString();
			fxFile.write(data.toUtf8());
			fxFile.flush();
			fxFile.close();
			return;
		}
	}
}

void EffectView::loadFxPreset()
{
	FileDialog fd(this, tr("Load preset"), "", tr("FX preset (*.lfxp)"));

	fd.setAcceptMode(FileDialog::AcceptOpen);
	fd.setFileMode(FileDialog::ExistingFile);
	fd.setNameFilter("FX presets (*.lfxp)");
	fd.setDirectory(ConfigManager::inst()->factoryPresetsDir());

	if ( fd.exec() == QDialog::Accepted &&
		!fd.selectedFiles().isEmpty() &&
		!fd.selectedFiles().first().isEmpty() )
	{
		QFile fxFile(fd.selectedFiles()[0]);
		if (fxFile.open(QFile::ReadOnly))
		{
			Effect * efx = effect();
			QDomDocument doc;
			doc.setContent( fxFile.readAll() );
			QDomNodeList roots = doc.elementsByTagName("lfxp");
			if ( roots.size() == 1 )
			{
				QDomElement lxfp = roots.at(0).toElement();
				if ( lxfp.attribute("name") == efx->descriptor()->name &&
					 lxfp.attribute("type") == QVariant::fromValue(efx->descriptor()->type).toString())
				{
					efx->loadSettings(lxfp);
				}
			}
			fxFile.close();
			return;
		}
	}
}
void EffectView::contextMenuEvent( QContextMenuEvent * )
{
	QPointer<CaptionMenu> contextMenu = new CaptionMenu( model()->displayName(), this );
	contextMenu->addAction( embed::getIconPixmap( "arp_up" ),
						tr( "Move &up" ),
						this, SLOT(moveUp()));
	contextMenu->addAction( embed::getIconPixmap( "arp_down" ),
						tr( "Move &down" ),
						this, SLOT(moveDown()));
	contextMenu->addSeparator();
	contextMenu->addAction( embed::getIconPixmap( "cancel" ),
						tr( "&Remove this plugin" ),
						this, SLOT(deletePlugin()));
	contextMenu->addSeparator();
	contextMenu->exec( QCursor::pos() );
	delete contextMenu;
}



void EffectView::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		m_dragging = true;
		m_opacityEffect->setOpacity(0.3);
		QCursor c(Qt::SizeVerCursor);
		QApplication::setOverrideCursor(c);
		update();
	}
}

void EffectView::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		m_dragging = false;
		m_opacityEffect->setOpacity(1);
		QApplication::restoreOverrideCursor();
		update();
	}
}

void EffectView::mouseMoveEvent(QMouseEvent* event)
{
	if (!m_dragging) { return; }
	if (event->pos().y() < 0)
	{
		moveUp();
	}
	else if (event->pos().y() > EffectView::DEFAULT_HEIGHT)
	{
		moveDown();
	}
}



void EffectView::paintEvent( QPaintEvent * )
{
	QPainter p( this );
	p.drawPixmap( 0, 0, m_bg );

	QFont f = pointSizeF( font(), 7.5f );
	f.setBold( true );
	p.setFont( f );

	QString elidedText = p.fontMetrics().elidedText( model()->displayName(), Qt::ElideRight, width() - 22 );

	p.setPen( palette().shadow().color() );
	p.drawText( 6, 55, elidedText );
	p.setPen( palette().text().color() );
	p.drawText( 5, 54, elidedText );
}




void EffectView::modelChanged()
{
	m_bypass->setModel( &effect()->m_enabledModel );
	m_wetDry->setModel( &effect()->m_wetDryModel );
	m_autoQuit->setModel( &effect()->m_autoQuitModel );
	m_gate->setModel( &effect()->m_gateModel );
}

} // namespace lmms::gui
