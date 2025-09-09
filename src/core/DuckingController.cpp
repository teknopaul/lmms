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
#include "AudioEngine.h"
#include "Song.h"


namespace lmms
{


DuckingController::DuckingController( Model * _parent ) :
	Controller( ControllerType::Ducking, _parent, tr( "Ducking Controller" ) ),
	m_baseModel( 0.33, 0.0, 1.0, 0.001, this, tr( "Base value" ) ),
	m_speedModel( 2.0, 0.01, 20.0, 0.0001, 20000.0, this, tr( "Oscillator speed" ) ),
	m_amountModel( 0.35, -1.0, 1.0, 0.005, this, tr( "Oscillator amount" ) ),
	m_phaseModel( 0.0, 0.0, 360.0, 4.0, this, tr( "Oscillator phase" ) ),
	m_waveModel( static_cast<int>(DuckingController::DuckShape::BezierU), 0, DuckingController::NumDuckShapes,
			this, tr( "Oscillator waveform" ) ),
	m_multiplierModel( 0, 0, 2, this, tr( "Frequency Multiplier" ) ),
	m_duration( 1000 ),
	m_phaseOffset( 0 ),
	m_currentPhase( 0 ),
	m_sampleFunction( &Oscillator::sinSample ),
	m_userDefSampleBuffer( new SampleBuffer ),
	m_oscillatorBezier(new OscillatorBezierU() )
{
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

	float oneUnit = 60000.0 / ( Engine::getSong()->getTempo() * 20000.0 );
	m_speedModel.setValue( oneUnit * 20.0 );

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
	m_phaseOffset = m_phaseModel.value() / 360.0;
	float phase = m_currentPhase + m_phaseOffset;
	float phasePrev = 0.0f;

	// roll phase up until we're in sync with period counter
	m_bufferLastUpdated++;
	if( m_bufferLastUpdated < s_periods )
	{
		int diff = s_periods - m_bufferLastUpdated;
		phase += static_cast<float>( Engine::audioEngine()->framesPerPeriod() * diff ) / m_duration;
		m_bufferLastUpdated += diff;
	}

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
		default:
		{
			if (m_sampleFunction != nullptr)
			{
				currentSample = m_sampleFunction(phase);
			}
		}
	}

		f = std::clamp(m_baseModel.value() + (*amountPtr * currentSample / 2.0f), 0.0f, 1.0f);

		phasePrev = phase;
		phase += 1.0 / m_duration;
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


void DuckingController::updateDuration()
{
	float newDurationF = Engine::audioEngine()->processingSampleRate() * m_speedModel.value();

	switch(m_multiplierModel.value() )
	{
		case 1:
			newDurationF /= 100.0;
			break;

		case 2:
			newDurationF *= 100.0;
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
