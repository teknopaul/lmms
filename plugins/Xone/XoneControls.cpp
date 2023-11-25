/*
 * XoneControls.cpp - controls for xone effect
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


#include <QDomElement>

#include "XoneControls.h"
#include "Xone.h"

namespace lmms
{

XoneControls::XoneControls( XoneEffect* effect ) :
	EffectControls( effect ),
	m_effect( effect ),
	m_freqModel( 20.0f, 0.0f, 14000.0f, 1.0f, this, tr( "Freq" ) ),
	m_wildModel( 0.01f, BasicFilters<>::minQ(), 6.5f, 0.1f, this, tr( "Wild" ) )
{
	connect( &m_freqModel, SIGNAL( dataChanged() ), effect, SLOT( controlChanged() ) );
	connect( &m_wildModel, SIGNAL( dataChanged() ), effect, SLOT( controlChanged() ) );
}


void XoneControls::changeControl()
{
//	engine::getSong()->setModified();
}




void XoneControls::loadSettings( const QDomElement& _this )
{
	m_freqModel.loadSettings( _this, "freq" );
	m_wildModel.loadSettings( _this, "wild" );
}




void XoneControls::saveSettings( QDomDocument& doc, QDomElement& _this )
{
	m_freqModel.saveSettings( doc, _this, "freq" ); 
	m_wildModel.saveSettings( doc, _this, "wild" );
}


} // namespace lmms


