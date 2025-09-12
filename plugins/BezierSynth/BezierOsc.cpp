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
			const float freq,
			const float detuning_div_samplerate,
			const float volume,
			FloatModel * mutateModel,
			const float attack,
			BezierOsc * sub_osc,
			SampleBuffer * user_wave,
			OscillatorBezierDefinition * bezierDef) :
	QObject(),
	m_waveAlgo(wave_algo),
	m_modulationAlgo(mod_algo),
	m_freq(freq),
	m_detuning_div_samplerate(detuning_div_samplerate),
	m_volume(volume),
	m_mutateModel(mutateModel),
	m_attack(attack),
	m_subOsc(sub_osc),
	m_phaseOffset(0),
	m_phase(0),
	m_sample_rate( Engine::audioEngine()->processingSampleRate() ),
	m_bezier(nullptr),
	m_userWave(user_wave),
	m_frames_played(0)
{
	if (m_waveAlgo == WaveAlgo::BezierSin) {
		m_bezier = new OscillatorBezierSin(mutateModel->value());
		// TODO broken why?
		auto ok = connect(m_mutateModel, &FloatModel::dataChanged, this, &BezierOsc::mutateChanged);
		if (!ok) qWarning("connect bug");
	}
	else if (m_waveAlgo == WaveAlgo::BezierZ) {
		m_bezier = new OscillatorBezierZ(mutateModel->value());
		// TODO broken why?
		auto ok = connect(m_mutateModel, &FloatModel::dataChanged, this, &BezierOsc::mutateChanged);
		if (!ok) qWarning("connect bug");
	}
	else if (m_waveAlgo == WaveAlgo::BezierUser) {
		m_bezier = new OscillatorBezierUser(bezierDef, mutateModel->value());
		auto ok = connect(m_mutateModel, &FloatModel::dataChanged, this, &BezierOsc::mutateChanged);
		if (!ok) qWarning("connect bug");
	}
}

void BezierOsc::mutateChanged()
{
	qDebug("BezierOsc::mutateChanged()");
	if (m_bezier != nullptr) {
		m_bezier->modulate(m_mutateModel->value());
	}
}

/**
 * @return from 1.0 to 0.0 vol multiplier
 */
sample_t BezierOsc::fadeOut(uint32_t frames_played, float seconds)
{
	sample_rate_t totframes = m_sample_rate * seconds;
	if (frames_played > totframes)  return 0.0f;
	if (frames_played == 0) return 1.0f;
	return std::exp(-5.0f * (static_cast<float>(frames_played) / totframes));
}

/**
 * @return from 0.0 to 1.0 vol multiplier
 */
inline sample_t fadeInExp(float t, float k = 5.0f)
{
	t = std::clamp(t, 0.0f, 1.0f);
	return 1.0f - std::exp(-k * t);        // 0 → 1
}
sample_t BezierOsc::fadeIn(uint32_t frames_played, float seconds)
{
	if (seconds < 1e-6f) return 1.0f;
	uint32_t totframes = m_sample_rate * seconds;
	if (frames_played > totframes) return 1.0f;
	if (frames_played == 0) return 0.0f;
	return fadeInExp(static_cast<float>(frames_played) / totframes);
}

/**
 * write the audio
 * param clean wipe the buffer clean first
 */
void BezierOsc::update(sampleFrame* ab, const fpp_t frames, bool clean)
{
	if (m_freq >= Engine::audioEngine()->processingSampleRate() / 2)
	{
		BufferManager::clear(ab, frames);
		return;
	}
	if (m_subOsc != nullptr)
	{
		switch (m_modulationAlgo)
		{
			case ModulationAlgo::SignalMix:
			default:
				updateMix(ab, frames, clean);
				break;
			case ModulationAlgo::AmplitudeModulation:
				updateAM(ab, frames, clean);
				break;
			case ModulationAlgo::FrequencyModulation:
				updateFM(ab, frames, clean);
				break;
		}
	}
	else
	{
		updateNoSub(ab, frames, clean);
	}
	// copy channel 0 to channel 1
	for( uint32_t frame = 0; frame < frames; ++frame )
	{
		ab[frame][1] = ab[frame][0];
	}
	m_frames_played = m_frames_played + (uint32_t)frames;
}




void BezierOsc::updateNoSub( sampleFrame * sampleArrays, const fpp_t frames, bool clean )
{
	switch( m_waveAlgo )
	{
		case WaveAlgo::Sine:
		default:
			updateNoSub<WaveAlgo::Sine>( sampleArrays, frames, clean );
			break;
		case WaveAlgo::Noise:
			updateNoSub<WaveAlgo::Noise>( sampleArrays, frames, clean );
			break;
		case WaveAlgo::BezierSin:
			updateNoSub<WaveAlgo::BezierSin>( sampleArrays, frames, clean );
			break;
		case WaveAlgo::BezierZ:
			updateNoSub<WaveAlgo::BezierZ>( sampleArrays, frames, clean );
			break;
		case WaveAlgo::Sample:
			updateNoSub<WaveAlgo::Sample>( sampleArrays, frames, clean );
			break;
	}
}






void BezierOsc::updateAM( sampleFrame * sampleArrays, const fpp_t frames, bool clean )
{
	switch( m_waveAlgo )
	{
		case WaveAlgo::Sine:
		default:
			updateAM<WaveAlgo::Sine>( sampleArrays, frames, clean );
			break;
		case WaveAlgo::BezierZ:
			updateAM<WaveAlgo::BezierZ>( sampleArrays, frames, clean );
			break;
		case WaveAlgo::BezierSin:
			updateAM<WaveAlgo::BezierSin>( sampleArrays, frames, clean );
			break;
		case WaveAlgo::Noise:
			updateAM<WaveAlgo::Noise>( sampleArrays, frames, clean );
			break;
		case WaveAlgo::Sample:
			updateAM<WaveAlgo::Sample>( sampleArrays, frames, clean );
			break;
	}
}


void BezierOsc::updateMix( sampleFrame * sampleArrays, const fpp_t frames, bool clean)
{
	switch( m_waveAlgo )
	{
		case WaveAlgo::Sine:
		default:
			updateMix<WaveAlgo::Sine>( sampleArrays, frames, clean );
			break;
		case WaveAlgo::Noise:
			updateMixNoise( sampleArrays, frames, clean );
			break;
		case WaveAlgo::Sample:
			updateMix<WaveAlgo::Sample>( sampleArrays, frames, clean );
			break;
		case WaveAlgo::BezierZ:
			updateMix<WaveAlgo::BezierZ>( sampleArrays, frames, clean );
			break;
		case WaveAlgo::BezierSin:
			updateMix<WaveAlgo::BezierSin>( sampleArrays, frames, clean );
			break;
	}
}



void BezierOsc::updateFM( sampleFrame * sampleArrays, const fpp_t frames, bool clean )
{
	switch( m_waveAlgo )
	{
		case WaveAlgo::Sine:
		default:
			updateFM<WaveAlgo::Sine>( sampleArrays, frames, clean );
			break;
		case WaveAlgo::BezierZ:
			updateFM<WaveAlgo::BezierZ>( sampleArrays, frames, clean );
			break;
		case WaveAlgo::BezierSin:
			updateFM<WaveAlgo::BezierSin>( sampleArrays, frames, clean );
			break;
		case WaveAlgo::Noise:
			updateFM<WaveAlgo::Noise>( sampleArrays, frames,  clean );
			break;
		case WaveAlgo::Sample:
			updateFM<WaveAlgo::Sample>( sampleArrays, frames, clean );
			break;
	}
}


// should be called every time phase-offset is changed...
inline void BezierOsc::recalcPhase()
{
	m_phase = absFraction( m_phase );
}

inline bool BezierOsc::syncOk( float osc_coeff )
{
	const float v1 = m_phase;
	m_phase += osc_coeff;
	// check whether m_phase is in next period
	return( floorf( m_phase ) > floorf( v1 ) );
}


float BezierOsc::syncInit( sampleFrame * sampleArrays, const fpp_t frames, bool clean )
{
	if( m_subOsc != nullptr )
	{
		m_subOsc->update( sampleArrays, frames, clean );
	}
	recalcPhase();
	return m_freq * m_detuning_div_samplerate;
}




// if we have no sub-osc, we can't do any modulation... just get our samples
template<BezierOsc::WaveAlgo W>
void BezierOsc::updateNoSub( sampleFrame * sampleArrays, const fpp_t frames, bool clean )
{
	recalcPhase();

	const float osc_coeff = m_freq * m_detuning_div_samplerate;

	for( uint32_t frame = 0; frame < frames; ++frame )
	{
		sample_t s = getSample<W>( m_phase )
				* m_volume
				* fadeIn(m_frames_played + frame + 1, m_attack);
		if (clean) {
			sampleArrays[frame][0] = s;
		} else {
			sampleArrays[frame][0] += s;
		}
		m_phase += osc_coeff;
	}

}

// if we have no sub-osc, we can't do any modulation... just get our samples
void BezierOsc::updateNoSubNoise( sampleFrame * sampleArrays, const fpp_t frames, bool clean )
{
	recalcPhase();

	const float osc_coeff = m_freq * m_detuning_div_samplerate;

	for( uint32_t frame = 0; frame < frames; ++frame )
	{
		sample_t s = noiseSample( m_phase )
				* m_volume
				* fadeOut(m_frames_played + frame + 1, 2);
		if (clean) {
			sampleArrays[frame][0] = s;
		} else {
			sampleArrays[frame][0] += s;
		}
		m_phase += osc_coeff;
	}

}


// do am by using sub-osc as modulator
template<BezierOsc::WaveAlgo W>
void BezierOsc::updateAM( sampleFrame * sampleArrays, const fpp_t frames, bool clean )
{
	m_subOsc->update( sampleArrays, frames, clean );
	recalcPhase();
	const float osc_coeff = m_freq * m_detuning_div_samplerate;

	for( uint32_t frame = 0; frame < frames; ++frame )
	{
		sample_t s = getSample<W>( m_phase )
				* m_volume
				* fadeIn(m_frames_played + frame + 1, m_attack);
		sampleArrays[frame][0] *= s;
		m_phase += osc_coeff;
	}
}


// do mix by using sub-osc as mix-sample
template<BezierOsc::WaveAlgo W>
void BezierOsc::updateMix( sampleFrame * sampleArrays, const fpp_t frames,
							bool clean )
{
	m_subOsc->update( sampleArrays, frames, clean );
	recalcPhase();
	const float osc_coeff = m_freq * m_detuning_div_samplerate;

	for( uint32_t frame = 0; frame < frames; ++frame )
	{
		sample_t s = getSample<W>( m_phase )
				* m_volume
				* fadeIn(m_frames_played + frame + 1, m_attack);
		sampleArrays[frame][0] += s;
		m_phase += osc_coeff;
	}
}

// do mix by using sub-osc as mix-sample
void BezierOsc::updateMixNoise( sampleFrame * sampleArrays, const fpp_t frames, bool clean)
{
	m_subOsc->update( sampleArrays, frames, clean );
	recalcPhase();
	const float osc_coeff = m_freq * m_detuning_div_samplerate;

	for( uint32_t frame = 0; frame < frames; ++frame )
	{
		sample_t s = noiseSample( m_phase )
				* m_volume
				* fadeOut(m_frames_played + frame + 1, 2.0f);
		sampleArrays[frame][0] += s;
		m_phase += osc_coeff;
	}
}

// do fm by using sub-osc as modulator
template<BezierOsc::WaveAlgo W>
void BezierOsc::updateFM( sampleFrame * sampleArrays, const fpp_t frames, bool clean )
{
	m_subOsc->update( sampleArrays, frames, clean );
	recalcPhase();
	const float osc_coeff = m_freq * m_detuning_div_samplerate;
	const float sampleRateCorrection = 44100.0f / m_sample_rate;

	for( uint32_t frame = 0; frame < frames; ++frame )
	{
		m_phase += sampleArrays[frame][0] * sampleRateCorrection;
		sample_t s = getSample<W>( m_phase )
				* m_volume
				* fadeIn(m_frames_played + frame + 1, m_attack);
		sampleArrays[frame][0] = s;
		m_phase += osc_coeff;
	}
}


template<>
inline sample_t BezierOsc::getSample<BezierOsc::WaveAlgo::Sine>(const float sample)
{
	const float current_freq = m_freq * m_detuning_div_samplerate * m_sample_rate;

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
inline sample_t BezierOsc::getSample<BezierOsc::WaveAlgo::Noise>( const float sample )
{
	return noiseSample( sample );
}

template<>
inline sample_t BezierOsc::getSample<BezierOsc::WaveAlgo::Sample>( const float sample )
{
	return userWaveSample( sample );
}

template<>
inline sample_t BezierOsc::getSample<BezierOsc::WaveAlgo::BezierZ>( const float sample )
{
	return bezierSample( sample );
}

template<>
inline sample_t BezierOsc::getSample<BezierOsc::WaveAlgo::BezierSin>( const float sample )
{
	return bezierSample( sample );
}

template<>
inline sample_t BezierOsc::getSample<BezierOsc::WaveAlgo::BezierUser>( const float sample )
{
	return bezierSample( sample );
}


} // namespace lmms
