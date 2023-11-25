/*
 * Xone.cpp - Low pass filter with resonance for a whole mix
 * Its just a standard low pass filter with Q but tuned to behave like an Xone mixer.
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

#include "Xone.h"

#include "embed.h"
#include "plugin_export.h"
#include "BasicFilters.h"

namespace lmms
{

extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT xone_plugin_descriptor =
{
	LMMS_STRINGIFY( PLUGIN_NAME ),
	"Xone",
	QT_TRANSLATE_NOOP( "PluginBrowser", "A low pass filter with resonance" ),
	"Teknopaul",
	0x0100,
	Plugin::Type::Effect,
	new PluginPixmapLoader("logo"),
	nullptr,
	nullptr,
};

}



XoneEffect::XoneEffect( Model* parent, const Descriptor::SubPluginFeatures::Key* key ) :
	Effect( &xone_plugin_descriptor, parent, key ),
	m_xoneControls( this )
{
	m_filter = new BasicFilters<2>( Engine::audioEngine()->processingSampleRate() );
	m_filter->setFilterType(BasicFilters<>::FilterType::Highpass_RC24);
	controlChanged();
}

void XoneEffect::controlChanged()
{
	float freq = m_xoneControls.m_freqModel.value();
	float wild = m_xoneControls.m_wildModel.value();

	m_filter->calcFilterCoeffs( freq, wild );
}


bool XoneEffect::processAudioBuffer( sampleFrame* buf, const fpp_t frames )
{
	if( !isEnabled() || !isRunning () )
	{
		return( false );
	}

	double outSum = 0.0;
	const float d = dryLevel();
	const float w = wetLevel();

	for( int f = 0; f < frames; ++f )
	{
		buf[f][0] = m_filter->update( buf[f][0], 0 );
		buf[f][1] = m_filter->update( buf[f][1], 1 );
	}

	for( fpp_t f = 0; f < frames; ++f )
	{

		auto s = std::array{buf[f][0], buf[f][1]};

		buf[f][0] = d * buf[f][0] + w * s[0];
		buf[f][1] = d * buf[f][1] + w * s[1];
		outSum += buf[f][0] * buf[f][0] + buf[f][1] * buf[f][1];
	}

	checkGate( outSum / frames );

	return isRunning();
}





extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main( Model* parent, void* data )
{
	return new XoneEffect( parent, static_cast<const Plugin::Descriptor::SubPluginFeatures::Key *>( data ) );
}

}

} // namespace lmms
