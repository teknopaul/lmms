/*
 * MidiSwing.cpp - Swing algo using fixed integer step adjustments that 
 *                 could be represented as midi. N.B. midi note is not changed
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

#include <QLabel>

#include "MidiSwing.h"

#include "Engine.h"

namespace lmms {

MidiSwing::MidiSwing(QObject * _parent) :
	QObject( _parent ),
	Groove()
{
}

MidiSwing::~MidiSwing()
{
}


void MidiSwing::apply( Note * _n )
{

	// Where are we in the beat
	int pos_in_beat =  _n->pos().getTicks() % (DefaultTicksPerBar / 4);

	// the Midi Swing algorthym.

	int pos_in_eigth = -1;
	if ( pos_in_beat >= 12 && pos_in_beat < 18 ) 
	{
		// 1st half of second quarter
		pos_in_eigth = pos_in_beat - 12;  // 0-5
	}
	else if ( pos_in_beat >= 36 && pos_in_beat < 42 )
	{
		// 1st half of third quarter
		pos_in_eigth = pos_in_beat - 36;  // 0-5
	}

	int swingTicks = applyMidiSwing(pos_in_eigth);

	_n->setNoteOffset(swingTicks * Engine::framesPerTick());

}

int MidiSwing::applyMidiSwing(int _pos_in_eigth)
{
	if (_pos_in_eigth < 0) return 0;
	if (_pos_in_eigth == 0) return 3;
	if (_pos_in_eigth == 1) return 3;
	if (_pos_in_eigth == 2) return 4;
	if (_pos_in_eigth == 3) return 4;
	if (_pos_in_eigth == 4) return 5;
	if (_pos_in_eigth == 5) return 5;
	return 0;

}

void MidiSwing::saveSettings( QDomDocument & _doc, QDomElement & _element )
{

}

void MidiSwing::loadSettings( const QDomElement & _this )
{

}

QWidget * MidiSwing::instantiateView( QWidget * _parent )
{
	return new lmms::gui::GrooveText(this, "Swing with amount set\nat exact midi ticks", _parent);
}

}
