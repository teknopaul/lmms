/*
 * XoneControlDialog.cpp - control dialog for xone effect
 *
 * Copyright (c) 2023 Teknopaul
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


#include "XoneControlDialog.h"
#include "XoneControls.h"
#include "embed.h"
#include "Knob.h"


namespace lmms::gui
{


XoneControlDialog::XoneControlDialog( XoneControls* controls ) :
	EffectControlDialog( controls )
{
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );
	setFixedSize( 100, 110 );

	auto freqKnob = new Knob(KnobType::Bright26, this);
	freqKnob ->move( 16, 10 );
	freqKnob->setVolumeKnob(true);
	freqKnob->setModel( &controls->m_freqModel );
	freqKnob->setLabel( tr( "F" ) );
	freqKnob->setHintText( tr( "F:" ) , "Hz" );

	auto wildKnob = new Knob(KnobType::Bright26, this);
	wildKnob -> move( 57, 10 );
	wildKnob->setModel( &controls->m_wildModel );
	wildKnob->setLabel( tr( "Wild" ) );
	wildKnob->setHintText( tr( "Wild:" ) , "" );

}


} // namespace lmms::gui
