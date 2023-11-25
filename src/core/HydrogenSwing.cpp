/*
 * HydrogenSwing.cpp - Swing algo that varies adjustments form 0-127
 *              The algorythm mimics Hydrogen drum machines swing feature.
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
#include "HydrogenSwing.h"
#include "Knob.h"
#include "lmms_basics.h"
#include "Note.h"
#include "Song.h"

#include "stdio.h"


namespace lmms
{

HydrogenSwing::HydrogenSwing(QObject * _parent) :
	QObject( _parent ),
	Groove()
{
	m_swingAmount = 0;
	m_swingFactor = 0;
	init();
	update();
}

HydrogenSwing::~HydrogenSwing()
{
}


void HydrogenSwing::init()
{
	Song * s = Engine::getSong();
	connect( s, SIGNAL(projectLoaded()),        this, SLOT(update()) );
	connect( s, SIGNAL(lengthChanged(int)),        this, SLOT(update()) );
	connect( s, SIGNAL(tempoChanged(bpm_t)),         this, SLOT(update()) );
	connect( s, SIGNAL(timeSignatureChanged(int, int)), this, SLOT(update()) );
}

int HydrogenSwing::amount()
{
	return m_swingAmount;
}

void HydrogenSwing::update()
{
	m_frames_per_tick =  Engine::framesPerTick();
}

void HydrogenSwing::setAmount(int _amount)
{

	if (_amount > 0 && _amount <= 127)
	{
		m_swingAmount = _amount;
		m_swingFactor =  (((float)m_swingAmount) / 127.0);
		emit swingAmountChanged(m_swingAmount);
	}
	else if (_amount  == 0)
	{
		m_swingAmount = 0;
		m_swingFactor =  0.0;
		emit swingAmountChanged(m_swingAmount);
	}
	else
	{
		m_swingAmount = 127;
		m_swingFactor =  1.0;
		emit swingAmountChanged(m_swingAmount);
	}

}

void HydrogenSwing::apply( Note * _n )
{

	// Where are we in the beat
	// 48 ticks to the beat, 192 ticks to the bar
	int pos_in_beat =  _n->pos().getTicks() % 48;


	// The Hydrogen Swing algorthym.
	// Guessed by turning the knob and watching the possitions change in Audacity.
	// Basically we delay (shift) notes on the the 2nd and 4th quarter of the beat.

	int pos_in_eigth = -1;
	if ( pos_in_beat >= 12 && pos_in_beat < 18 )
	{
		// 1st half of second quarter
		pos_in_eigth = pos_in_beat - 12;  // 0-5
	}
	else  if ( pos_in_beat >= 36 && pos_in_beat < 42 )
	{
		// 1st half of third quarter
		pos_in_eigth = pos_in_beat - 36;  // 0-5
	}

	if ( pos_in_eigth >= 0 )
	{

		float ticks_to_shift = ((pos_in_eigth - 6) * -m_swingFactor);

		f_cnt_t frames_to_shift = (int)(ticks_to_shift * m_frames_per_tick);

		_n->setNoteOffset(frames_to_shift);

	}

}



void HydrogenSwing::saveSettings( QDomDocument & _doc, QDomElement & _element )
{
	_element.setAttribute("swingAmount", m_swingAmount);
}

void HydrogenSwing::loadSettings( const QDomElement & _this )
{
	bool ok;
	int amount =  _this.attribute("swingAmount").toInt(&ok);
	if (ok)
	{
		setAmount(amount);
	}
	else
	{
		setAmount(0);
	}
}

QWidget * HydrogenSwing::instantiateView( QWidget * _parent )
{
	return new lmms::gui::HydrogenSwingView(this, _parent);
}

}

namespace lmms::gui
{

// VIEW //

HydrogenSwingView::HydrogenSwingView(HydrogenSwing * _hy_swing, QWidget * _parent) :
	QWidget( _parent )
{
	m_nobModel = new FloatModel(0.0, 0.0, 127.0, 1.0); // Unused
	m_nob = new Knob(KnobType::Bright26, this, "swingFactor");
	m_nob->setModel( m_nobModel );
	m_nob->setLabel( tr( "Swinginess" ) );
	m_nob->setEnabled(true);
	m_nobModel->setValue(_hy_swing->amount());

	m_hy_swing = _hy_swing;

	connect(m_nob, SIGNAL(sliderMoved(float)), this, SLOT(valueChanged(float)));
	connect(m_nobModel, SIGNAL( dataChanged() ), this, SLOT(modelChanged()) );
}

HydrogenSwingView::~HydrogenSwingView()
{
	delete m_nob;
	delete m_nobModel;
}

void HydrogenSwingView::modelChanged()
{
	m_hy_swing->setAmount((int)m_nobModel->value());
}

void HydrogenSwingView::valueChanged(float _f) // this value passed is gibberish
{
	m_hy_swing->setAmount((int)m_nobModel->value());
}


}
