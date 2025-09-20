/*
 * LfoController.cpp - implementation of class controller which handles
 *                      remote-control of AutomatableModels
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail.com>
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

#include <QDomElement>


#include "DuckingController.h"
#include "OscillatorBezierU.h"
#include "OscillatorBezierV.h"
#include "OscillatorBezierHhRide.h"
#include "AudioEngine.h"
#include "Song.h"


namespace lmms
{

// Essentially this in an LFO that instead of getting out of sync due to wierdness of float math in C++
// and the fact that sample rate / beats per second may not be an exact integer
// calls syncToSong(); every time its asked for samples.
// LFO precision over the song is Engine::getSong()->getFrames() ) / m_duration where m_duration is a float.
// TODO UI should be in whole beats
// TODO beat math should be sane
// TODO UI should go 1 2 4 8 16 32 in a little digital display
// TODO x2 button works for all my usecases but is fugly

DuckingController::DuckingController( Model * _parent ) :
	Controller( ControllerType::Ducking, _parent, tr( "Ducking Controller" ) ),
	m_baseModel( 0.333, 0.0, 1.0, 0.001, this, tr( "Base value" ) ),
	m_speedModel( 2.0, 0.01, 20.0, 0.0001, 20000.0, this, tr( "Oscillator speed" ) ),
	m_amountModel( 0.333, -1.0, 1.0, 0.005, this, tr( "Oscillator amount" ) ),
	m_phaseModel( 0.0, 0.0, 360.0, 4.0, this, tr( "Oscillator phase" ) ),
	m_waveModel( static_cast<int>(DuckingController::DuckShape::BezierU), 0, DuckingController::NumDuckShapes,
			this, tr( "Oscillator waveform" ) ),
	m_multiplierModel( 0, 0, 2, this, tr( "Frequency Multiplier" ) ),
	m_duration( 1000 ),
	m_phaseOffset( 0 ),
	m_currentPhase( 0 ),
	m_sampleFunction( &Oscillator::sinSample ),
	m_userDefSampleBuffer( new SampleBuffer ),
	m_oscillatorBezier( nullptr )
{
	if (m_waveModel.value() == static_cast<int>(DuckingController::DuckShape::BezierU)) {
		m_oscillatorBezier = new OscillatorBezierU();
	} else	if (m_waveModel.value() == static_cast<int>(DuckingController::DuckShape::BezierHhRide)) {
		m_oscillatorBezier = new OscillatorBezierHhRide();
	} else	if (m_waveModel.value() == static_cast<int>(DuckingController::DuckShape::BezierV)) {
		m_oscillatorBezier = new OscillatorBezierV();
	}
	setSampleExact( true );
	connect( &m_waveModel, SIGNAL(dataChanged()),
			this, SLOT(updateSampleFunction()), Qt::DirectConnection );

	connect( &m_speedModel, SIGNAL(dataChanged()),
			this, SLOT(updateDuration()), Qt::DirectConnection );
	connect( &m_multiplierModel, SIGNAL(dataChanged()),
			this, SLOT(updateDuration()), Qt::DirectConnection );
	connect( Engine::audioEngine(), SIGNAL(sampleRateChanged()),
			this, SLOT(updateDuration()));

	connect( Engine::getSong(), SIGNAL(playbackStateChanged()),
			this, SLOT(updatePhase()));
	connect( Engine::getSong(), SIGNAL(playbackPositionChanged()),
			this, SLOT(updatePhase()));

	updateDuration();
}


DuckingController::~DuckingController()
{
	sharedObject::unref( m_userDefSampleBuffer );
	m_baseModel.disconnect( this );
	m_speedModel.disconnect( this );
	m_amountModel.disconnect( this );
	m_phaseModel.disconnect( this );
	m_waveModel.disconnect( this );
	m_multiplierModel.disconnect( this );
}


void DuckingController::updateValueBuffer()
{
	// dont move if not playing, sit at full vol
	if ( Engine::getSong()->isPaused() || Engine::getSong()->isStopped() ) {
		float amount = m_amountModel.value();
		for( float& f : m_valueBuffer )
		{
			f = std::clamp(m_baseModel.value() + (amount * 1.0f / 2.0f), 0.0f, 1.0f);
		}
		return;
	}

	// sync to song every time, N.B only works for fixed bpm songs
	syncToSong();

	// support phase since sin is useless without it, sould set m_phaseOffset = 270.0f for sin?
	m_phaseOffset = m_phaseModel.value() / 360.0;
	float phase = m_currentPhase + m_phaseOffset;

	// roll phase up until we're in sync with period counter
	// TODO necessary anymore?
	m_bufferLastUpdated++;
	if( m_bufferLastUpdated < s_periods )
	{
		int diff = s_periods - m_bufferLastUpdated;
		phase += static_cast<float>( Engine::audioEngine()->framesPerPeriod() * diff ) / m_duration;
		m_bufferLastUpdated += diff;
	}

	// Seems to be support for varying amount over the duration of this sample fill (unnesessary for ducking?)
	float amount = m_amountModel.value();
	ValueBuffer *amountBuffer = m_amountModel.valueBuffer();
	int amountInc = amountBuffer ? 1 : 0;
	float *amountPtr = amountBuffer ? &(amountBuffer->values()[ 0 ] ) : &amount;
	DuckingController::DuckShape waveshape = static_cast<DuckingController::DuckShape>(m_waveModel.value());

	for( float& f : m_valueBuffer )
	{
		float currentSample = 0;
		switch (waveshape)
		{
			case DuckingController::DuckShape::UserDefined:
			{
				currentSample = m_userDefSampleBuffer->userWaveSample(phase);
				break;
			}
			case DuckingController::DuckShape::BezierU:
			{
				currentSample = m_oscillatorBezier->oscSample(phase);
				break;
			}
			case DuckingController::DuckShape::BezierHhRide:
			{
				currentSample = m_oscillatorBezier->oscSample(phase);
				break;
			}
			case DuckingController::DuckShape::BezierV:
			{
				currentSample = m_oscillatorBezier->oscSample(phase);
				break;
			}
			default:
			{
				if (m_sampleFunction != nullptr)
				{
					currentSample = m_sampleFunction(phase);
				}
			}
		}

		f = std::clamp(m_baseModel.value() + (*amountPtr * currentSample / 2.0f), 0.0f, 1.0f);

		phase += 1.0f / m_duration;
		// since LFO does get out of sync, this check ensures that at the end of one phase we dont start another
		// seems to work
		if ( phase >= 1.0f ) phase = 1.0f;
		amountPtr += amountInc;
	}

	m_currentPhase = absFraction(phase - m_phaseOffset);
	m_bufferLastUpdated = s_periods;
}

void DuckingController::updatePhase()
{
	m_currentPhase = ( Engine::getSong()->getFrames() ) / m_duration;
	m_bufferLastUpdated = s_periods - 1;
}

void DuckingController::syncToSong()
{
	// TODO lazy, function presumes fixed bpm
	m_currentPhase = absFraction(( Engine::getSong()->getFrames() ) / m_duration);
	m_bufferLastUpdated = s_periods - 1;
}

void DuckingController::tempoToBeat()
{
	// default to one beat at current bpm (I dont understand the math)
	float oneUnit = 60000.0f / ( Engine::getSong()->getTempo() * 20000.0f );
	m_speedModel.setValue( oneUnit * 20.0f );
	m_multiplierModel.setValue(0);
}

void DuckingController::tempoToPhrase()
{
	// default to 32 beats at current bpm (I dont understand the math)
	float oneUnit = 60000.0f / ( Engine::getSong()->getTempo() * 20000.0f );
	m_speedModel.setValue( oneUnit * 20.0f * 16.0f );
	m_multiplierModel.setValue(2);
}


void DuckingController::updateDuration()
{
	float newDurationF = Engine::audioEngine()->processingSampleRate() * m_speedModel.value();

	switch(m_multiplierModel.value() )
	{
		case 1:
			newDurationF /= 2.0;
			break;

		case 2:
			newDurationF *= 2.0;
			break;

		default:
			break;
	}

	m_duration = newDurationF;
}

void DuckingController::updateSampleFunction()
{
	switch( static_cast<DuckingController::DuckShape>(m_waveModel.value()) )
	{
		case DuckingController::DuckShape::Sine:
		default:
			m_sampleFunction = &Oscillator::sinSample;
			break;
		case DuckingController::DuckShape::Triangle:
			m_sampleFunction = &Oscillator::triangleSample;
			break;
		case DuckingController::DuckShape::Saw:
			m_sampleFunction = &Oscillator::sawSample;
			break;
		case DuckingController::DuckShape::Square:
			m_sampleFunction = &Oscillator::squareSample;
			break;
		case DuckingController::DuckShape::BezierU:
			m_sampleFunction = nullptr;
			// TODO memory leak vs crash are we synced?
			delete m_oscillatorBezier;
			m_oscillatorBezier = new OscillatorBezierU();
			tempoToBeat();
			break;
		case DuckingController::DuckShape::BezierV:
			m_sampleFunction = nullptr;
			// TODO memory leak vs crash are we synced?
			delete m_oscillatorBezier;
			m_oscillatorBezier = new OscillatorBezierV();
			tempoToBeat();
			break;
		case DuckingController::DuckShape::BezierHhRide:
			m_sampleFunction = nullptr;
			// TODO memory leak vs crash are we synced?
			delete m_oscillatorBezier;
			m_oscillatorBezier = new OscillatorBezierHhRide();
			tempoToPhrase();
			break;
		case DuckingController::DuckShape::UserDefined:
			m_sampleFunction = nullptr;
			/*TODO: If C++11 is allowed, should change the type of
			 m_sampleFunction be std::function<sample_t(const float)>
			 and use the line below:
			*/
			//m_sampleFunction = &(m_userDefSampleBuffer->userWaveSample)
			break;
	}
}



void DuckingController::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	Controller::saveSettings( _doc, _this );

	m_baseModel.saveSettings( _doc, _this, "base" );
	m_speedModel.saveSettings( _doc, _this, "speed" );
	m_amountModel.saveSettings( _doc, _this, "amount" );
	m_phaseModel.saveSettings( _doc, _this, "phase" );
	m_waveModel.saveSettings( _doc, _this, "wave" );
	m_multiplierModel.saveSettings( _doc, _this, "multiplier" );
	_this.setAttribute( "userwavefile" , m_userDefSampleBuffer->audioFile() );
}



void DuckingController::loadSettings( const QDomElement & _this )
{
	Controller::loadSettings( _this );

	m_baseModel.loadSettings( _this, "base" );
	m_speedModel.loadSettings( _this, "speed" );
	m_amountModel.loadSettings( _this, "amount" );
	m_phaseModel.loadSettings( _this, "phase" );
	m_waveModel.loadSettings( _this, "wave" );
	m_multiplierModel.loadSettings( _this, "multiplier" );
	m_userDefSampleBuffer->setAudioFile( _this.attribute("userwavefile" ) );

	updateSampleFunction();
}



QString DuckingController::nodeName() const
{
	return( "duckingcontroller" );
}



gui::ControllerDialog * DuckingController::createDialog( QWidget * _parent )
{
	return new gui::DuckingControllerDialog( this, _parent );
}


} // namespace lmms
