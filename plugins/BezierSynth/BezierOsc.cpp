/*
 * BezierOscillator.cpp - Oscillator supporint bezier mutations
 *
 * Copyright (c) 2025 teknopaul
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

#include "BezierOsc.h"

#include <algorithm>
#if !defined(__MINGW32__) && !defined(__MINGW64__)
	#include <thread>
#endif

#include "BufferManager.h"
#include "Engine.h"
#include "AudioEngine.h"
#include "AutomatableModel.h"

namespace lmms
{


BezierOsc::BezierOsc(
			const WaveAlgo wave_algo,
			const ModulationAlgo mod_algo,
			const float &freq,
			const float &detuning_div_samplerate,
			const float &volume,
			BezierOsc * sub_osc,
			SampleBuffer * user_wave) :
	m_waveAlgo(wave_algo),
	m_modulationAlgo(mod_algo),
	m_freq(freq),
	m_detuning_div_samplerate(detuning_div_samplerate),
	m_volume(volume),
	m_subOsc(sub_osc),
	m_phaseOffset(0),
	m_phase(0),
	m_isModulator(false),
	m_bezier(nullptr),
	m_userWave(user_wave)
{
	if (wave_algo == BezierOsc::WaveAlgo::BezierZ) {
		m_bezier = new OscillatorBezierZ();
	}
}




void BezierOsc::update(sampleFrame* ab, const fpp_t frames, const ch_cnt_t chnl, bool modulator)
{
	if (m_freq >= Engine::audioEngine()->processingSampleRate() / 2)
	{
		BufferManager::clear(ab, frames);
		return;
	}
	// If this oscillator is used to PM or PF modulate another oscillator, take a note.
	// The sampling functions will check this variable and avoid using band-limited
	// wavetables, since they contain ringing that would lead to unexpected results.
	m_isModulator = modulator;
	if (m_subOsc != nullptr)
	{
		switch (m_modulationAlgo)
		{
			case ModulationAlgo::SignalMix:
			default:
				updateMix(ab, frames, chnl);
				break;
			case ModulationAlgo::AmplitudeModulation:
				updateAM(ab, frames, chnl);
				break;
			case ModulationAlgo::FrequencyModulation:
				updateFM(ab, frames, chnl);
				break;
		}
	}
	else
	{
		updateNoSub(ab, frames, chnl);
	}
}




void BezierOsc::updateNoSub( sampleFrame * _ab, const fpp_t _frames,
							const ch_cnt_t _chnl )
{
	switch( m_waveAlgo )
	{
		case WaveAlgo::Sine:
		default:
			updateNoSub<WaveAlgo::Sine>( _ab, _frames, _chnl );
			break;
		case WaveAlgo::Noise:
			updateNoSub<WaveAlgo::Noise>( _ab, _frames, _chnl );
			break;
		case WaveAlgo::BezierZ:
			updateNoSub<WaveAlgo::BezierZ>( _ab, _frames, _chnl );
			break;
		case WaveAlgo::Sample:
			updateNoSub<WaveAlgo::Sample>( _ab, _frames, _chnl );
			break;
	}
}






void BezierOsc::updateAM( sampleFrame * _ab, const fpp_t _frames,
							const ch_cnt_t _chnl )
{
	switch( m_waveAlgo )
	{
		case WaveAlgo::Sine:
		default:
			updateAM<WaveAlgo::Sine>( _ab, _frames, _chnl );
			break;
		case WaveAlgo::BezierZ:
			updateAM<WaveAlgo::BezierZ>( _ab, _frames, _chnl );
			break;
			// TODO these dont exist
		case WaveAlgo::Noise:
			updateAM<WaveAlgo::Noise>( _ab, _frames, _chnl );
			break;
		case WaveAlgo::Sample:
			updateAM<WaveAlgo::Sample>( _ab, _frames, _chnl );
			break;
	}
}


void BezierOsc::updateMix( sampleFrame * _ab, const fpp_t _frames,
							const ch_cnt_t _chnl )
{
	switch( m_waveAlgo )
	{
		case WaveAlgo::Sine:
		default:
			updateMix<WaveAlgo::Sine>( _ab, _frames, _chnl );
			break;
		case WaveAlgo::Noise:
			updateMix<WaveAlgo::Noise>( _ab, _frames, _chnl );
			break;
		case WaveAlgo::Sample:
			updateMix<WaveAlgo::Sample>( _ab, _frames, _chnl );
			break;
		case WaveAlgo::BezierZ:
			updateMix<WaveAlgo::BezierZ>( _ab, _frames, _chnl );
			break;
	}
}



void BezierOsc::updateFM( sampleFrame * _ab, const fpp_t _frames,
							const ch_cnt_t _chnl )
{
	switch( m_waveAlgo )
	{
		case WaveAlgo::Sine:
		default:
			updateFM<WaveAlgo::Sine>( _ab, _frames, _chnl );
			break;
		case WaveAlgo::BezierZ:
			updateFM<WaveAlgo::BezierZ>( _ab, _frames, _chnl );
			break;
			// TODO these dont exist
		case WaveAlgo::Noise:
			updateFM<WaveAlgo::Noise>( _ab, _frames, _chnl );
			break;
		case WaveAlgo::Sample:
			updateFM<WaveAlgo::Sample>( _ab, _frames, _chnl );
			break;
	}
}


// should be called every time phase-offset is changed...
inline void BezierOsc::recalcPhase()
{
	m_phase = absFraction( m_phase );
}

inline bool BezierOsc::syncOk( float _osc_coeff )
{
	const float v1 = m_phase;
	m_phase += _osc_coeff;
	// check whether m_phase is in next period
	return( floorf( m_phase ) > floorf( v1 ) );
}


float BezierOsc::syncInit( sampleFrame * _ab, const fpp_t _frames,
						const ch_cnt_t _chnl )
{
	if( m_subOsc != nullptr )
	{
		m_subOsc->update( _ab, _frames, _chnl );
	}
	recalcPhase();
	return m_freq * m_detuning_div_samplerate;
}




// if we have no sub-osc, we can't do any modulation... just get our samples
template<BezierOsc::WaveAlgo W>
void BezierOsc::updateNoSub( sampleFrame * _ab, const fpp_t _frames, const ch_cnt_t _chnl )
{
	recalcPhase();
	const float osc_coeff = m_freq * m_detuning_div_samplerate;

	for( fpp_t frame = 0; frame < _frames; ++frame )
	{
		_ab[frame][_chnl] = getSample<W>( m_phase ) * m_volume;
		m_phase += osc_coeff;
	}
}


// do am by using sub-osc as modulator
template<BezierOsc::WaveAlgo W>
void BezierOsc::updateAM( sampleFrame * _ab, const fpp_t _frames,
							const ch_cnt_t _chnl )
{
	m_subOsc->update( _ab, _frames, _chnl, false );
	recalcPhase();
	const float osc_coeff = m_freq * m_detuning_div_samplerate;

	for( fpp_t frame = 0; frame < _frames; ++frame )
	{
		_ab[frame][_chnl] *= getSample<W>( m_phase ) * m_volume;
		m_phase += osc_coeff;
	}
}


// do mix by using sub-osc as mix-sample
template<BezierOsc::WaveAlgo W>
void BezierOsc::updateMix( sampleFrame * _ab, const fpp_t _frames,
							const ch_cnt_t _chnl )
{
	m_subOsc->update( _ab, _frames, _chnl, false );
	recalcPhase();
	const float osc_coeff = m_freq * m_detuning_div_samplerate;

	for( fpp_t frame = 0; frame < _frames; ++frame )
	{
		_ab[frame][_chnl] += getSample<W>( m_phase ) * m_volume;
		m_phase += osc_coeff;
	}
}

// do fm by using sub-osc as modulator
template<BezierOsc::WaveAlgo W>
void BezierOsc::updateFM( sampleFrame * _ab, const fpp_t _frames,
							const ch_cnt_t _chnl )
{
	m_subOsc->update( _ab, _frames, _chnl, true );
	recalcPhase();
	const float osc_coeff = m_freq * m_detuning_div_samplerate;
	const float sampleRateCorrection = 44100.0f / Engine::audioEngine()->processingSampleRate();

	for( fpp_t frame = 0; frame < _frames; ++frame )
	{
		m_phase += _ab[frame][_chnl] * sampleRateCorrection;
		_ab[frame][_chnl] = getSample<W>( m_phase ) * m_volume;
		m_phase += osc_coeff;
	}
}


template<>
inline sample_t BezierOsc::getSample<BezierOsc::WaveAlgo::Sine>(const float sample)
{
	const float current_freq = m_freq * m_detuning_div_samplerate * Engine::audioEngine()->processingSampleRate();

	if (current_freq < OscillatorConstants::MAX_FREQ)
	{
		return sinSample(sample);
	}
	else
	{
		return 0;
	}
}


template<>
inline sample_t BezierOsc::getSample<BezierOsc::WaveAlgo::Noise>( const float _sample )
{
	return noiseSample( _sample );
}




template<>
inline sample_t BezierOsc::getSample<BezierOsc::WaveAlgo::Sample>( const float _sample )
{
	return userWaveSample(_sample);
}


template<>
inline sample_t BezierOsc::getSample<BezierOsc::WaveAlgo::BezierZ>(
							const float _sample )
{
	return bezierSample(_sample);
}


} // namespace lmms
