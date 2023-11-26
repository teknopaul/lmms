/*
 * HalfSwing.cpp - Swing algo that varies adjustments form 0-127
 *              The algorythm is just the latter half of the HydrogenSwing groove..
 *
 * Copyright (c) 2004-2014 teknopaul <teknopaul/at/users.sourceforge.net>
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
#include <QtCore/QObject>
#include <QtXml/QDomElement>
#include <QLabel>

#include "Engine.h"
#include "Groove.h"
#include "HalfSwing.h"
#include "Knob.h"
#include "lmms_basics.h"
#include "Note.h"
#include "Song.h"

#include "stdio.h"

namespace lmms
{


HalfSwing::HalfSwing(QObject * _parent) :
	QObject( _parent ),
	Groove(),
	m_swingAmountModel(0, 0 , 127, 1.0, nullptr, "swing amount"),
	m_swingFactor( 0.0 )
{
	connect(&m_swingAmountModel, SIGNAL( dataChanged() ), this, SLOT( updateAmount() ) );
}

HalfSwing::~HalfSwing()
{
}



void HalfSwing::updateAmount()
{
	if (m_swingAmountModel.value() == 0)
	{
		m_swingFactor = 0.0;
	}
	else
	{
		m_swingFactor =  m_swingAmountModel.value() / 127.0;
	}
}

void HalfSwing::apply( Note * _n )
{

	// Where are we in the beat
	// 48 ticks to the beat, 192 ticks to the bar
	int pos_in_beat =  _n->pos().getTicks() % (DefaultTicksPerBar / 4);


	// The Half Swing algorthym.
	// Basically we delay (shift) notes on the the 4th quarter of the beat.

	int pos_in_eigth = -1;
	if ( pos_in_beat >= 36 && pos_in_beat < 42 )
	{
		// 1st half of third quarter
		pos_in_eigth = pos_in_beat - 36;  // 0-5
	}

	if ( pos_in_eigth >= 0 )
	{

		float ticks_to_shift = ((pos_in_eigth - 6) * -m_swingFactor);

		f_cnt_t frames_to_shift = (int)(ticks_to_shift * Engine::framesPerTick());

		_n->setNoteOffset(frames_to_shift);
	}
}



void HalfSwing::saveSettings( QDomDocument & _doc, QDomElement & _element )
{
	m_swingAmountModel.saveSettings(_doc, _element, "swingAmount");
}

void HalfSwing::loadSettings( const QDomElement & _element )
{
	m_swingAmountModel.loadSettings(_element, "swingAmount");
}

QWidget * HalfSwing::instantiateView( QWidget * _parent )
{
	return new lmms::gui::HalfSwingView(this, _parent);
}

}


namespace lmms::gui
{

// VIEW //

HalfSwingView::HalfSwingView(HalfSwing * swing, QWidget * _parent) :
	QWidget( _parent )
{
	m_nob = new Knob(KnobType::Bright26, this, "swing amount");
	m_nob->setModel( &swing->m_swingAmountModel );
	m_nob->setLabel( tr( "Swinginess" ) );
	m_nob->setEnabled(true);
}

HalfSwingView::~HalfSwingView()
{
}


}
