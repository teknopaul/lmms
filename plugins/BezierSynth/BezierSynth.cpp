/*
 * BezierSynth.cpp - Audio synthesis using Bezier curve math.
 *
 * Copyright (c) 2025 teknopaul <teknopaul/at/whatevs.net>
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


#include <QDebug>
#include <QDomElement>
#include <QLabel>

#include "BezierSynth.h"
#include "AudioEngine.h"
#include "AutomatableButton.h"
#include "debug.h"
#include "Engine.h"
#include "InstrumentTrack.h"
#include "Knob.h"
#include "NotePlayHandle.h"
#include "Oscillator.h"
#include "PixmapButton.h"
#include "SampleBuffer.h"
#include "PathUtil.h"
#include "BezierOsc.h"
#include "OscillatorBezier.h"

#include "embed.h"
#include "plugin_export.h"

namespace lmms
{


extern "C"
{


/**
 * A synth that uses bezier curves to generate sound.
 * Has 2 prinscipal oscillators, one can AM FM or mix modulate the other.
 * Each Bezier curve oscillator support a mutate knob to modulate the sound wave in some way
 * by applying some changes tot he bezier vectors used to generate sounds.
 */
Plugin::Descriptor PLUGIN_EXPORT beziersynth_plugin_descriptor =
{
	LMMS_STRINGIFY( PLUGIN_NAME ),
	"BezierSynth",
	QT_TRANSLATE_NOOP( "PluginBrowser", "Audio Synth using bezier curves" ),
	"teknopaul <teknopaul/at/whatevs.net>",
	0x0110,
	Plugin::Type::Instrument,
	new PluginPixmapLoader( "logo" ),
	nullptr,
	nullptr,
};

}

// this synth has 4 of these
// osc1 is the core sound wave
// osc2 is either a layered wave or a modulation wave
// osc3 is a noise layer
// osc4 is a waveform loaded as an audio file.
const int OSC_WAVE1 = 0;
const int OSC_WAVE2 = 0;
const int OSC_NOISE = 0;
const int OSC_SAMPLE = 0;


BezierSynthOscillatorObject::BezierSynthOscillatorObject( Model * _parent, int _idx ) :
	Model( _parent ),
	m_oscIndex(_idx),
	m_volumeModel( DefaultVolume / 2, MinVolume, MaxVolume, 1.0f, this, tr( "Osc %1 volume" ).arg( _idx+1 ) ),
	m_mutateModel( 0.0f, 0.0f, 1.0f, 0.01f, this, tr("Mutate")),
	m_coarseModel( -_idx * KeysPerOctave, -2 * KeysPerOctave, 2 * KeysPerOctave, 1.0f, this, tr( "Osc %1 coarse detuning" ).arg( _idx+1 ) ),
	m_fineModel( 0.0f, -100.0f, 100.0f, 1.0f, this, tr( "Osc %1 fine detuning" ).arg( _idx+1 ) ),
	m_attackModel( 0.0f, 0.0f, 2.0f, 0.01f, this, tr("Attack")),
	m_waveAlgoModel( static_cast<int>(BezierOsc::WaveAlgo::BezierZ), 0,
			BezierOsc::NumWaveAlgos-1, this,
			tr( "Bezier wave algo %1" ).arg( _idx+1 ) ),
	m_modulationAlgoModel( static_cast<int>(BezierOsc::ModulationAlgo::SignalMix), 0,
				BezierOsc::NumModulationAlgos-1, this,
				tr( "Modulation type %1" ).arg( _idx+1 ) ),
	m_sampleBuffer( new SampleBuffer ),
	m_bezierDefinition( new OscillatorBezierDefinition ),
	m_waveName(""),
	m_volume( 0.0f ),
	m_detuning( 0.0f ),
	m_dirScroller()
{
	if (m_oscIndex == OSC_NOISE) {
		m_fineModel.setValue(0);
		m_waveAlgoModel.setValue( static_cast<int>(BezierOsc::WaveAlgo::Noise) );
		m_modulationAlgoModel.setValue( static_cast<int>(BezierOsc::ModulationAlgo::SignalMix) );
	}
	if (m_oscIndex == OSC_SAMPLE) {
		m_fineModel.setValue(0);
		m_waveAlgoModel.setValue( static_cast<int>(BezierOsc::WaveAlgo::Sample) );
		m_modulationAlgoModel.setValue( static_cast<int>(BezierOsc::ModulationAlgo::SignalMix) );
	}

	// Connect knobs with Oscillators' inputs
	connect( &m_volumeModel, SIGNAL( dataChanged() ), this, SLOT( updateVolume() ), Qt::DirectConnection );
	updateVolume();

	connect( &m_coarseModel, SIGNAL( dataChanged() ), this, SLOT( updateDetuning() ), Qt::DirectConnection );

	if (m_oscIndex == OSC_WAVE1 || OSC_WAVE2) {
		connect( &m_fineModel, SIGNAL( dataChanged() ), this, SLOT( updateDetuning() ), Qt::DirectConnection );
		connect( &m_mutateModel, &FloatModel::dataChanged, this, &BezierSynthOscillatorObject::updateMutate , Qt::DirectConnection );
		updateMutate();
		connect( &m_attackModel, SIGNAL( dataChanged() ), this, SLOT( updateAttack() ), Qt::DirectConnection );
		updateAttack();
	}
	updateDetuning();
}


BezierSynthOscillatorObject::~BezierSynthOscillatorObject()
{
	sharedObject::unref( m_sampleBuffer );
}


void BezierSynthOscillatorObject::oscUserDefWaveDblClick()
{
	QString fileName = m_bezierDefinition->openSvgFile();
	if( fileName != "" )
	{
		m_waveAlgoModel.value(static_cast<int>(BezierOsc::WaveAlgo::BezierUser));
		// TODO:
		// wire up m_waveName to UI
		m_dirScroller.setFile(fileName);
		qWarning("set wave '%s'", PathUtil::toAbsolute(fileName).toStdString().c_str());
		m_waveName = m_bezierDefinition->getName();
		m_waveFile = fileName;
		qDebug() << "loaded: " << m_bezierDefinition->getName();
	}
}

void BezierSynthOscillatorObject::oscUserDefWaveNext()
{
	QString newFile = m_dirScroller.next();
	if ( ! newFile.isEmpty() )
	{
		qWarning("setting wave '%s'", newFile.toStdString().c_str());
		QFileInfo fInfo(PathUtil::toAbsolute(m_bezierDefinition->getFile()));
		int rv = m_bezierDefinition->loadFromSVG( PathUtil::toShortestRelative(fInfo.absolutePath() + QDir::separator() + newFile) );
		if (rv == 0) {
			m_waveAlgoModel.value(static_cast<int>(BezierOsc::WaveAlgo::BezierUser));
			m_waveName = m_bezierDefinition->getName();
			m_waveFile = newFile;
			qDebug() << "loaded: " << m_bezierDefinition->getName();
		}
	}
}


void BezierSynthOscillatorObject::oscUserDefWavePrev()
{
	QString newFile = m_dirScroller.prev();
	if ( ! newFile.isEmpty() )
	{
		qWarning("setting wave '%s' current='%s'", newFile.toStdString().c_str(), "todo");

		QFileInfo fInfo(PathUtil::toAbsolute(m_bezierDefinition->getFile()));
		int rv = m_bezierDefinition->loadFromSVG( PathUtil::toShortestRelative(fInfo.absolutePath() + QDir::separator() + newFile) );
		if (rv == 0) {
			m_waveAlgoModel.value(static_cast<int>(BezierOsc::WaveAlgo::BezierUser));
			m_waveName = m_bezierDefinition->getName();
			m_waveFile = newFile;
			qDebug() << "loaded: " << m_waveName;
		}
	}
}


void BezierSynthOscillatorObject::oscUserDefSampleDblClick()
{
	QString fileName = m_sampleBuffer->openAndSetWaveformFile();
	if( fileName != "" )
	{
		// TODO:
		//m_usrWaveBtn->setToolTip(m_sampleBuffer->audioFile());
		m_dirScroller.setFile(fileName);
		qWarning("set wave '%s'", PathUtil::toAbsolute(fileName).toStdString().c_str());
	}
}

void BezierSynthOscillatorObject::oscUserDefSampleNext()
{
	QString newFile = m_dirScroller.next();
	if ( ! newFile.isEmpty() )
	{
		qWarning("setting wave '%s'", newFile.toStdString().c_str());
		QFileInfo fInfo(PathUtil::toAbsolute(m_sampleBuffer->audioFile()));
		m_sampleBuffer->setAudioFile( PathUtil::toShortestRelative(fInfo.absolutePath() + QDir::separator() + newFile) );
	}
}


void BezierSynthOscillatorObject::oscUserDefSamplePrev()
{
	QString newFile = m_dirScroller.prev();
	if ( ! newFile.isEmpty() )
	{
		qWarning("setting wave '%s' current='%s'", newFile.toStdString().c_str(), m_sampleBuffer->audioFile().toStdString().c_str());
		QFileInfo fInfo(PathUtil::toAbsolute(m_sampleBuffer->audioFile()));
		m_sampleBuffer->setAudioFile( PathUtil::toShortestRelative(fInfo.absolutePath() + QDir::separator() + newFile) );
	}
}

void BezierSynthOscillatorObject::updateVolume()
{
	m_volume = m_volumeModel.value() / 100.0f;
}


void BezierSynthOscillatorObject::updateMutate()
{
}

void BezierSynthOscillatorObject::updateAttack()
{
}

void BezierSynthOscillatorObject::updateDetuning()
{
	m_detuning = powf( 2.0f, ( (float)m_coarseModel.value() * 100.0f
				+ (float)m_fineModel.value() ) / 1200.0f )
				/ Engine::audioEngine()->processingSampleRate();
}




BezierSynth::BezierSynth( InstrumentTrack * _instrument_track ) :
	Instrument( _instrument_track, &beziersynth_plugin_descriptor )
{
	m_osc1 = new BezierSynthOscillatorObject( this, 0 );
	m_osc2 = new BezierSynthOscillatorObject( this, 1 );
	m_osc_noise = new BezierSynthOscillatorObject( this, 2 );
	m_osc_sample = new BezierSynthOscillatorObject( this, 3 );

	connect( Engine::audioEngine(), SIGNAL( sampleRateChanged() ), this, SLOT( updateAllDetuning() ) );
}




void BezierSynth::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	// TODO i and is should be hardcoded and descriptive
	// osc1
	int i = 0;
	QString is = QString::number( i );
	m_osc1->m_volumeModel.saveSettings( _doc, _this, "vol" + is );
	m_osc1->m_coarseModel.saveSettings( _doc, _this, "coarse" + is );
	m_osc1->m_fineModel.saveSettings( _doc, _this, "fine" + is );
	m_osc1->m_mutateModel.saveSettings( _doc, _this, "mutate" + is );
	m_osc1->m_attackModel.saveSettings( _doc, _this, "attack" + is );
	m_osc1->m_waveAlgoModel.saveSettings( _doc, _this, "wavealgo" + is );
	if (BezierOsc::WaveAlgo::BezierUser == static_cast<BezierOsc::WaveAlgo>(m_osc1->m_waveAlgoModel.value())) {
		_this.setAttribute("waveFile" + is, m_osc1->m_waveFile);
	}
	m_osc1->m_modulationAlgoModel.saveSettings( _doc, _this, "modalgo" + is );

	// osc2
	i = 1;
	is = QString::number( i );
	m_osc2->m_volumeModel.saveSettings( _doc, _this, "vol" + is );
	m_osc2->m_coarseModel.saveSettings( _doc, _this, "coarse" + is );
	m_osc2->m_fineModel.saveSettings( _doc, _this, "fine" + is );
	m_osc2->m_mutateModel.saveSettings( _doc, _this, "mutate" + is );
	m_osc2->m_attackModel.saveSettings( _doc, _this, "attack" + is );
	m_osc2->m_waveAlgoModel.saveSettings( _doc, _this, "wavealgo" + is );
	if (BezierOsc::WaveAlgo::BezierUser == static_cast<BezierOsc::WaveAlgo>(m_osc2->m_waveAlgoModel.value())) {
		_this.setAttribute("waveFile" + is, m_osc2->m_waveFile);
	}

	// osc_noise
	i = 2;
	is = QString::number( i );
	m_osc_noise->m_volumeModel.saveSettings( _doc, _this, "vol" + is );

	// osc_sample
	i = 3;
	is = QString::number( i );
	m_osc_sample->m_volumeModel.saveSettings( _doc, _this, "vol" + is );
	m_osc_sample->m_coarseModel.saveSettings( _doc, _this, "coarse" + is );
	m_osc_sample->m_attackModel.saveSettings( _doc, _this, "attack" + is );
	m_osc_sample->m_playModel.saveSettings( _doc, _this, "play" + is );
	_this.setAttribute( "userwavefile" + is, m_osc_sample->m_sampleBuffer->audioFile() );
}




void BezierSynth::loadSettings( const QDomElement & _this )
{
	int i = 0;
	QString is = QString::number( i );
	m_osc1->m_volumeModel.loadSettings( _this, "vol" + is );
	m_osc1->m_coarseModel.loadSettings( _this, "coarse" + is );
	m_osc1->m_fineModel.loadSettings( _this, "fine" + is );
	m_osc1->m_mutateModel.loadSettings( _this, "mutate" + is );
	m_osc1->m_attackModel.loadSettings( _this, "attack" + is );
	m_osc1->m_waveAlgoModel.loadSettings( _this, "wavealgo" + is );
	m_osc1->m_modulationAlgoModel.loadSettings( _this, "modalgo" + is );
	if (BezierOsc::WaveAlgo::BezierUser == static_cast<BezierOsc::WaveAlgo>(m_osc1->m_waveAlgoModel.value())) {
		m_osc1->m_waveFile = _this.attribute("waveFile" + is);
		int rv = m_osc1->m_bezierDefinition->loadFromSVG( PathUtil::toShortestRelative(m_osc1->m_waveFile) );
		if (rv == 0) {
			m_osc1->m_waveName = m_osc1->m_bezierDefinition->getName();
			qDebug() << "loaded: " << m_osc1->m_waveName;
		}
	}

	i = 1;
	is = QString::number( i );
	m_osc2->m_volumeModel.loadSettings( _this, "vol" + is );
	m_osc2->m_coarseModel.loadSettings( _this, "coarse" + is );
	m_osc2->m_fineModel.loadSettings( _this, "fine" + is );
	m_osc2->m_mutateModel.loadSettings( _this, "mutate" + is );
	m_osc2->m_attackModel.loadSettings( _this, "attack" + is );
	m_osc2->m_waveAlgoModel.loadSettings( _this, "wavealgo" + is );
	if (BezierOsc::WaveAlgo::BezierUser == static_cast<BezierOsc::WaveAlgo>(m_osc2->m_waveAlgoModel.value())) {
		m_osc2->m_waveFile = _this.attribute("waveFile" + is);
		int rv = m_osc2->m_bezierDefinition->loadFromSVG( PathUtil::toShortestRelative(m_osc2->m_waveFile) );
		if (rv == 0) {
			m_osc2->m_waveName = m_osc2->m_bezierDefinition->getName();
			qDebug() << "loaded: " << m_osc2->m_waveName;
		}
	}

	i = 2;
	is = QString::number( i );
	m_osc_noise->m_volumeModel.loadSettings( _this, "vol" + is );

	i = 3;
	is = QString::number( i );
	m_osc_sample->m_volumeModel.loadSettings( _this, "vol" + is );
	m_osc_sample->m_coarseModel.loadSettings( _this, "coarse" + is );
	m_osc_sample->m_attackModel.loadSettings( _this, "attack" + is );
	m_osc_sample->m_playModel.loadSettings( _this, "play" + is );
	m_osc_sample->m_sampleBuffer->setAudioFile( _this.attribute( "userwavefile" + is ) );
}


QString BezierSynth::nodeName() const
{
	return( beziersynth_plugin_descriptor.name );
}


void BezierSynth::playNote( NotePlayHandle * _n, sampleFrame * _working_buffer )
{
	BezierOsc * osc;
	oscPtr * oscPtr;
	bool playWholeSample = m_osc_sample->m_playModel.value();
	if (!_n->m_pluginData)
	{
		// TODO no need for left and right they do the same math twice
		auto oscs = std::array<BezierOsc*, 4>{};

		if ( ! playWholeSample ) {
			// osc_sample
			oscs[3] = new BezierOsc(
					BezierOsc::WaveAlgo::Sample,
					BezierOsc::ModulationAlgo::SignalMix,
					_n->frequency(),
					m_osc_sample->m_detuning,
					m_osc_sample->m_volume,
					nullptr,
					m_osc_sample->m_attackModel.value(),
					nullptr, // no sub osc
					m_osc_sample->m_sampleBuffer );
		} else {
			oscs[3] = nullptr;
		}

		// osc_noise
		oscs[2] = new BezierOsc(
				BezierOsc::WaveAlgo::Noise,
				BezierOsc::ModulationAlgo::SignalMix,
				_n->frequency(),
				0.f,
				m_osc_noise->m_volume,
				nullptr,
				0.0f,
				oscs[3],
				nullptr );


		// osc2
		oscs[1] = new BezierOsc(
				static_cast<BezierOsc::WaveAlgo>( m_osc2->m_waveAlgoModel.value() ),
				BezierOsc::ModulationAlgo::SignalMix,
				_n->frequency(),
				m_osc2->m_detuning,
				m_osc2->m_volume,
				&m_osc2->m_mutateModel,
				m_osc2->m_attackModel.value(),
				oscs[2],
				nullptr,
				m_osc2->m_bezierDefinition);

		// osc1
		oscs[0] = new BezierOsc(
				static_cast<BezierOsc::WaveAlgo>( m_osc1->m_waveAlgoModel.value() ),
				static_cast<BezierOsc::ModulationAlgo>( m_osc1->m_modulationAlgoModel.value() ),
				_n->frequency(),
				m_osc1->m_detuning,
				m_osc1->m_volume,
				&m_osc1->m_mutateModel,
				m_osc1->m_attackModel.value(),
				oscs[1],
				nullptr,
				m_osc1->m_bezierDefinition);

		oscPtr = new struct oscPtr;
		_n->m_pluginData = oscPtr;
		oscPtr->osc = oscs[0];
		oscPtr->playState = 0;
	}

	osc = static_cast<struct oscPtr *>( _n->m_pluginData )->osc;

	const fpp_t frames = _n->framesLeftForCurrentPeriod();
	const f_cnt_t offset = _n->noteOffset();

	if ( playWholeSample ) {
		playSample( _n, _working_buffer + offset, m_osc_sample->m_sampleBuffer );
	}

	osc->update( _working_buffer + offset, frames, !playWholeSample );

	applyFadeIn(_working_buffer, _n);
	applyRelease( _working_buffer, _n );

}

void BezierSynth::playSample( NotePlayHandle * n, sampleFrame * working_buffer, SampleBuffer * sampleBuffer )
{
	const fpp_t frames = n->framesLeftForCurrentPeriod();
	const f_cnt_t offset = n->noteOffset();

	handleState * playState;
	if( !static_cast<oscPtr *>( n->m_pluginData )->playState )
	{
		playState = new handleState( false, SRC_LINEAR );
		static_cast<oscPtr *>( n->m_pluginData )->playState = playState;
		sampleBuffer->setFrequency(n->frequency());
		sampleBuffer->setFrequency(440.0f);
		sampleBuffer->setAmplification(m_osc_sample->getVolume());
	} else {
		playState = static_cast<oscPtr *>( n->m_pluginData )->playState;
	}

	if( ! n->isFinished() )
	{
		// this makes not play around the correct freequency , why??
		//float freq = n->frequency() - instrumentTrack()->baseFreq();
		float freq = 440.0f;
		if (sampleBuffer->play( working_buffer + offset,
						playState,
						frames, freq,
						SampleBuffer::LoopMode::Off )) {
			applyRelease( working_buffer, n );
		} else {
			memset( working_buffer, 0, ( frames + offset ) * sizeof( sampleFrame ) );
		}
	}
}


void BezierSynth::deleteNotePluginData( NotePlayHandle * n )
{
	delete static_cast<BezierOsc *>( static_cast<oscPtr *>( n->m_pluginData )->osc );
	delete static_cast<handleState *>( static_cast<oscPtr *>( n->m_pluginData )->playState );
	delete static_cast<oscPtr *>( n->m_pluginData );
}



gui::PluginView* BezierSynth::instantiateView( QWidget * parent )
{
	return new gui::BezierSynthView( this, parent );
}


void BezierSynth::updateAllDetuning()
{
	m_osc1->updateDetuning();
	m_osc2->updateDetuning();
	m_osc_noise->updateDetuning();
	m_osc_sample->updateDetuning();
}


namespace gui
{


class BezierSynthKnob : public Knob
{
public:
	BezierSynthKnob( QWidget * parent ) : Knob( KnobType::Dark28, parent )
	{
		setFixedSize( 30, 35 );
	}
};

class BezierMutateKnob : public Knob
{
public:
	BezierMutateKnob( QWidget * parent ) : Knob( KnobType::Bright26, parent )
	{
		setFixedSize( 30, 35 );
		setProperty("outerColor", QColor(0, 100, 0));
		setProperty("arcInctiveColor", QColor(0, 100, 0));
		setProperty("arcActiveColor", QColor(220, 0, 0));
		setProperty("lineInctiveColor", QColor(0, 100, 0));
		setProperty("lineActiveColor", QColor(220, 0, 0));
	}
};

// 82, 109


BezierSynthView::BezierSynthView( Instrument * instrument, QWidget * parent ) :
	InstrumentViewFixedSize( instrument, parent )
{
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );

	// modulations buttons, how osc2 modulates osc1

	const int mod_x = 66;
	const int mod1_y = 50;

	auto mix_osc_btn = new PixmapButton(this, nullptr);
	mix_osc_btn->move( mod_x + 0, mod1_y );
	mix_osc_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "mix_active" ) );
	mix_osc_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "mix_inactive" ) );
	mix_osc_btn->setToolTip(tr("Mix output of waves 1 & 2"));

	auto am_osc_btn = new PixmapButton(this, nullptr);
	am_osc_btn->move( mod_x + 35, mod1_y );
	am_osc_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "am_active" ) );
	am_osc_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "am_inactive" ) );
	am_osc_btn->setToolTip(tr("Modulate amplitude of wave 1 by wave 2"));

	auto fm_osc_btn = new PixmapButton(this, nullptr);
	fm_osc_btn->move( mod_x + 70, mod1_y );
	fm_osc_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "fm_active" ) );
	fm_osc_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "fm_inactive" ) );
	fm_osc_btn->setToolTip(tr("Modulate frequency of wave 1 by wave 2"));

	m_modBtnGrp = new automatableButtonGroup( this );
	m_modBtnGrp->addButton( mix_osc_btn );
	m_modBtnGrp->addButton( am_osc_btn );
	m_modBtnGrp->addButton( fm_osc_btn );

	const int osc_1y = 10;
	const int osc_2y = 73;
	const int osc_3y = 123;
	const int osc_4y = 173;

	int i = 0;
	int knob_y = osc_1y + 10;
	int knob_x = 5;

	// OSCILLATOR 1

	// setup volume-knob
	auto vol1 = new BezierSynthKnob(this);
	vol1->setVolumeKnob( true );
	vol1->move( knob_x, knob_y );
	vol1->setHintText( tr( "Volume %1:" ).arg( i+1 ), "%" );

	// setup coarse-knob
	Knob * course1 = new BezierSynthKnob(this);
	course1->move( knob_x + 40, knob_y );
	course1->setHintText( tr( "Coarse detuning %1:" ).arg( i + 1 ) , " " + tr( "semitones" ) );

	// setup knob for fine-detuning
	Knob * fine1 = new BezierSynthKnob(this);
	fine1->move(  knob_x + 72, knob_y );
	fine1->setHintText( tr( "Fine detuning %1:" ).arg( i + 1 ), " " + tr( "cents" ) );

	// setup mutate-knob
	auto mutate1 = new BezierMutateKnob(this);
	mutate1->setVolumeKnob( true );
	mutate1->move(  knob_x + 104, knob_y );
	mutate1->setHintText( tr( "Wave %1 mutate:" ).arg( i+1 ), "" );

	// setup attack-knob
	auto attack1 = new BezierSynthKnob(this);
	attack1->setVolumeKnob( true );
	attack1->move(  knob_x + 134, knob_y );
	attack1->setHintText( tr( "Attack %1:" ).arg( i+1 ), "" );

	m_osc1WaveName.setParent(this);
	m_osc1WaveName.move(knob_x + 175, knob_y + 5);
	m_osc1WaveName.setGeometry(knob_x + 175, knob_y + 5, 60, 23);

	int btn_y = 0;
	int btn_x = 65;
	int x = 0;
	auto sin_wave_btn1 = new PixmapButton(this, nullptr);
	sin_wave_btn1->move( btn_x + ( x++ * 15), btn_y );
	sin_wave_btn1->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "sin_shape_active" ) );
	sin_wave_btn1->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "sin_shape_inactive" ) );
	sin_wave_btn1->setToolTip( tr( "Sine wave" ) );

	auto noise_btn1 = new PixmapButton(this, nullptr);
	noise_btn1->move( btn_x + ( x++ * 15), btn_y );
	noise_btn1->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "white_noise_shape_active" ) );
	noise_btn1->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "white_noise_shape_inactive" ) );
	noise_btn1->setToolTip( tr( "White noise" ) );

	auto beziersin_btn1 = new PixmapButton(this, nullptr);
	beziersin_btn1->move( btn_x + ( x++ * 15), btn_y );
	beziersin_btn1->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "beziersin_wave_active" ) );
	beziersin_btn1->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "beziersin_wave_inactive" ) );
	beziersin_btn1->setToolTip(tr("BezierSin2Tri wave"));

	auto bezierz_btn1 = new PixmapButton(this, nullptr);
	bezierz_btn1->move( btn_x + ( x++ * 15), btn_y );
	bezierz_btn1->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "bezierz_wave_active" ) );
	bezierz_btn1->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "bezierz_wave_inactive" ) );
	bezierz_btn1->setToolTip(tr("BezierZ wave"));

	auto uwb1 = new PixmapButton(this, nullptr);
	uwb1->move( 199, btn_y );
	uwb1->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "usr_shape_active" ) );
	uwb1->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "usr_shape_inactive" ) );
	uwb1->setToolTip(tr("User-defined wave"));

	auto lrn1 = new LeftRightNav(this);
	lrn1->setCursor( QCursor( Qt::PointingHandCursor ) );
	lrn1->move( 215, btn_y );

	auto wabg1 = new automatableButtonGroup(this);

	wabg1->addButton( sin_wave_btn1 );
	wabg1->addButton( noise_btn1 );
	wabg1->addButton( beziersin_btn1 );
	wabg1->addButton( bezierz_btn1 );
	wabg1->addButton( uwb1 );


	m_osc1Knobs = BezierOscKnobs( vol1, course1, fine1, mutate1, attack1, nullptr, wabg1, uwb1, lrn1 );

	// OSCILLATOR 2

	i = 1;
	knob_y = osc_2y + 10;

	// setup volume-knob
	auto vol2 = new BezierSynthKnob(this);
	vol2->setVolumeKnob( true );
	vol2->move( knob_x, knob_y );
	vol2->setHintText( tr( "Wave %1 volume:" ).arg( i + 1 ), "%" );

	// setup coarse-knob
	Knob * course2 = new BezierSynthKnob(this);
	course2->move( knob_x + 40, knob_y );
	course2->setHintText( tr( "Wave %1 coarse detuning:" ).arg( i + 1 ) , " " + tr( "semitones" ) );

	// setup knob for fine-detuning
	Knob * fine2 = new BezierSynthKnob(this);
	fine2->move(  knob_x + 72, knob_y );
	fine2->setHintText( tr( "Wave %1 fine detuning:" ).arg( i + 1 ), " " + tr( "cents" ) );

	// setup mutate-knob
	auto mutate2 = new BezierMutateKnob(this);
	mutate2->setVolumeKnob( true );
	mutate2->move(  knob_x + 104, knob_y );
	mutate2->setHintText( tr( "Wave %1 mutate:" ).arg( i+1 ), "" );

	// setup attack-knob
	auto attack2 = new BezierSynthKnob(this);
	attack2->setVolumeKnob( true );
	attack2->move(  knob_x + 134, knob_y );
	attack2->setHintText( tr( "Attack %1 :" ).arg( i+1 ), "" );

	btn_y = 63;

	m_osc2WaveName.setParent(this);
	m_osc2WaveName.move(knob_x + 175, knob_y + 5);
	m_osc2WaveName.setGeometry(knob_x + 175, knob_y + 5, 60, 23);

	x = 0;
	auto sin_wave_btn2 = new PixmapButton(this, nullptr);
	sin_wave_btn2->move( btn_x + ( x++ * 15), btn_y );
	sin_wave_btn2->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "sin_shape_active" ) );
	sin_wave_btn2->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "sin_shape_inactive" ) );
	sin_wave_btn2->setToolTip( tr( "Sine wave" ) );

	auto noise_btn2 = new PixmapButton(this, nullptr);
	noise_btn2->move( btn_x + ( x++ * 15), btn_y );
	noise_btn2->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "white_noise_shape_active" ) );
	noise_btn2->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "white_noise_shape_inactive" ) );
	noise_btn2->setToolTip( tr( "White noise" ) );

	auto beziersin_btn2 = new PixmapButton(this, nullptr);
	beziersin_btn2->move( btn_x + ( x++ * 15), btn_y );
	beziersin_btn2->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "beziersin_wave_active" ) );
	beziersin_btn2->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "beziersin_wave_inactive" ) );
	beziersin_btn2->setToolTip(tr("BezierSin wave"));

	auto bezierz_btn2 = new PixmapButton(this, nullptr);
	bezierz_btn2->move( btn_x + ( x++ * 15), btn_y );
	bezierz_btn2->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "bezierz_wave_active" ) );
	bezierz_btn2->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "bezierz_wave_inactive" ) );
	bezierz_btn2->setToolTip(tr("BezierZ wave"));

	auto uwb2 = new PixmapButton(this, nullptr);
	uwb2->move( 199, btn_y );
	uwb2->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "usr_shape_active" ) );
	uwb2->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "usr_shape_inactive" ) );
	uwb2->setToolTip(tr("User-defined wave"));

	auto lrn2 = new LeftRightNav(this);
	lrn2->setCursor( QCursor( Qt::PointingHandCursor ) );
	lrn2->move( 215, btn_y );

	auto wabg2 = new automatableButtonGroup(this);

	wabg2->addButton( sin_wave_btn2 );
	wabg2->addButton( noise_btn2 );
	wabg2->addButton( beziersin_btn2 );
	wabg2->addButton( bezierz_btn2 );
	wabg2->addButton( uwb2 );

	m_osc2Knobs = BezierOscKnobs( vol2, course2, fine2, mutate2, attack2, nullptr, wabg2, uwb2, lrn2 );


	// NOISE

	i = 2;
	knob_y = osc_3y + 10;

	// setup volume-knob
	auto vol3 = new BezierSynthKnob(this);
	vol3->setVolumeKnob( true );
	vol3->move( knob_x, knob_y );
	vol3->setHintText( tr( "Osc %1 volume:" ).arg( i+1 ), "%" );

	m_oscNoiseKnobs = BezierOscKnobs( vol3, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr );


	// SAMPLE

	i = 3;
	knob_y = osc_4y + 10;


	// setup volume-knob
	auto vol4 = new BezierSynthKnob(this);
	vol4->setVolumeKnob( true );
	vol4->move( knob_x, knob_y );
	vol4->setHintText( tr( "Osc %1 volume:" ).arg( i+1 ), "%" );

	// setup coarse-knob
	Knob * course4 = new BezierSynthKnob(this);
	course4->move( knob_x + 40, knob_y );
	course4->setHintText( tr( "Osc %1 coarse detuning:" ).arg( i + 1 ) , " " + tr( "semitones" ) );

	// setup attack-knob
	auto attack4 = new BezierSynthKnob(this);
	attack4->setVolumeKnob( true );
	attack4->move(  knob_x + 70, knob_y );
	attack4->setHintText( tr( "Attack %1 :" ).arg( i+1 ), "" );


	btn_y = 163;

	LedCheckBox * play4 = new LedCheckBox(this);
	play4->move(230, 180);
	play4->setToolTip("play whole sample, not a wave form");
	// TODO default clicks

	auto uwb4 = new PixmapButton(this, nullptr);
	uwb4->move( 199, btn_y );
	uwb4->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "usr_shape_active" ) );
	uwb4->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "usr_shape_inactive" ) );
	uwb4->setToolTip(tr("User-defined wave"));

	auto lrn4 = new LeftRightNav(this);
	lrn4->setCursor( QCursor( Qt::PointingHandCursor ) );
	lrn4->move( 215, btn_y );

	auto wabg4 = new automatableButtonGroup(this);

	wabg4->addButton( uwb4 );

	m_oscSampleKnobs = BezierOscKnobs( vol4, course4, nullptr, nullptr, attack4, play4, wabg4, uwb4, lrn4 );

}


void BezierSynthView::modelChanged()
{
	auto t = castModel<BezierSynth>();
	m_modBtnGrp->setModel( &t->m_osc1->m_modulationAlgoModel );

	// osc1
	m_osc1Knobs.m_volKnob->setModel( &t->m_osc1->m_volumeModel );
	m_osc1Knobs.m_coarseKnob->setModel( &t->m_osc1->m_coarseModel );
	m_osc1Knobs.m_fineKnob->setModel( &t->m_osc1->m_fineModel );
	m_osc1Knobs.m_mutateKnob->setModel( &t->m_osc1->m_mutateModel );
	m_osc1Knobs.m_attackKnob->setModel( &t->m_osc1->m_attackModel );
	m_osc1Knobs.m_waveAlgoBtnGrp->setModel( &t->m_osc1->m_waveAlgoModel );
	m_osc1WaveName.setText(t->m_osc1->m_waveName);

	connect( m_osc1Knobs.m_userWaveButton, SIGNAL( doubleClicked() ), t->m_osc1, SLOT( oscUserDefWaveDblClick() ) );
	connect( m_osc1Knobs.m_userWaveSwitcher, SIGNAL( onNavLeft() ),   t->m_osc1, SLOT( oscUserDefWavePrev() ));
	connect( m_osc1Knobs.m_userWaveSwitcher, SIGNAL( onNavRight() ),  t->m_osc1, SLOT( oscUserDefWaveNext() ));

	// osc2
	m_osc2Knobs.m_volKnob->setModel( &t->m_osc2->m_volumeModel );
	m_osc2Knobs.m_coarseKnob->setModel( &t->m_osc2->m_coarseModel );
	m_osc2Knobs.m_fineKnob->setModel( &t->m_osc2->m_fineModel );
	m_osc2Knobs.m_mutateKnob->setModel( &t->m_osc2->m_mutateModel );
	m_osc2Knobs.m_attackKnob->setModel( &t->m_osc2->m_attackModel );
	m_osc2Knobs.m_waveAlgoBtnGrp->setModel( &t->m_osc2->m_waveAlgoModel );
	m_osc2WaveName.setText(t->m_osc2->m_waveName);

	connect( m_osc2Knobs.m_userWaveButton, SIGNAL( doubleClicked() ), t->m_osc2, SLOT( oscUserDefWaveDblClick() ) );
	connect( m_osc2Knobs.m_userWaveSwitcher, SIGNAL( onNavLeft() ),   t->m_osc2, SLOT( oscUserDefWavePrev() ));
	connect( m_osc2Knobs.m_userWaveSwitcher, SIGNAL( onNavRight() ),  t->m_osc2, SLOT( oscUserDefWaveNext() ));

	// noise
	m_oscNoiseKnobs.m_volKnob->setModel( &t->m_osc_noise->m_volumeModel );

	// sample
	m_oscSampleKnobs.m_volKnob->setModel( &t->m_osc_sample->m_volumeModel );
	m_oscSampleKnobs.m_coarseKnob->setModel( &t->m_osc_sample->m_coarseModel );
	m_oscSampleKnobs.m_attackKnob->setModel( &t->m_osc_sample->m_attackModel );
	m_oscSampleKnobs.m_playLed->setModel( &t->m_osc_sample->m_playModel );

	connect( m_oscSampleKnobs.m_userWaveButton, SIGNAL( doubleClicked() ), t->m_osc_sample, SLOT( oscUserDefSampleDblClick() ) );
	connect( m_oscSampleKnobs.m_userWaveSwitcher, SIGNAL( onNavLeft() ),   t->m_osc_sample, SLOT( oscUserDefSamplePrev() ));
	connect( m_oscSampleKnobs.m_userWaveSwitcher, SIGNAL( onNavRight() ),  t->m_osc_sample, SLOT( oscUserDefSampleNext() ));

}


} // namespace gui


extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main( Model* model, void * )
{
	return new BezierSynth( static_cast<InstrumentTrack *>( model ) );
}

}



} // namespace lmms
