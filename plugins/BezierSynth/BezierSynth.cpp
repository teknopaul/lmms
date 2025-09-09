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



#include <QDomElement>

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
// osc2 is eithe a layered wave ior a modulation wave
// osc3 is a noise wave
// osc4 is a sample

BezierSynthOscillatorObject::BezierSynthOscillatorObject( Model * _parent, int _idx ) :
	Model( _parent ),
	m_oscIndex(_idx),
	m_volumeModel( DefaultVolume / 2, MinVolume, MaxVolume, 1.0f, this, tr( "Osc %1 volume" ).arg( _idx+1 ) ),
	m_mutateModel( 0.0f, 0.0f, 1.0f, 0.01f, this, tr("Mutate")),
	m_coarseModel( -_idx * KeysPerOctave, -2 * KeysPerOctave, 2 * KeysPerOctave, 1.0f, this, tr( "Osc %1 coarse detuning" ).arg( _idx+1 ) ),
	m_fineModel( 0.0f, -100.0f, 100.0f, 1.0f, this, tr( "Osc %1 fine detuning" ).arg( _idx+1 ) ),
	m_waveAlgoModel( static_cast<int>(BezierOsc::WaveAlgo::BezierZ), 0,
			BezierOsc::NumWaveAlgos-1, this,
			tr( "Bezier wave algo %1" ).arg( _idx+1 ) ),
	m_modulationAlgoModel( static_cast<int>(BezierOsc::ModulationAlgo::SignalMix), 0,
				BezierOsc::NumModulationAlgos-1, this,
				tr( "Modulation type %1" ).arg( _idx+1 ) ),
	m_sampleBuffer( new SampleBuffer ),
	m_volumeLeft( 0.0f ),
	m_volumeRight( 0.0f ),
	m_detuning( 0.0f ),
	m_dirScroller()
{
	if (m_oscIndex == 2) {
		m_waveAlgoModel.setValue( static_cast<int>(BezierOsc::WaveAlgo::Noise) );
		m_modulationAlgoModel.setValue( static_cast<int>(BezierOsc::ModulationAlgo::SignalMix) );
	}
	// Connect knobs with Oscillators' inputs
	connect( &m_volumeModel, SIGNAL( dataChanged() ), this, SLOT( updateVolume() ), Qt::DirectConnection );
	updateVolume();

	connect( &m_coarseModel, SIGNAL( dataChanged() ), this, SLOT( updateDetuning() ), Qt::DirectConnection );
	connect( &m_fineModel, SIGNAL( dataChanged() ), this, SLOT( updateDetuning() ), Qt::DirectConnection );
	updateDetuning();

	connect( &m_mutateModel, SIGNAL( dataChanged() ), this, SLOT( updateMutate() ), Qt::DirectConnection );
	updateMutate();
}


BezierSynthOscillatorObject::~BezierSynthOscillatorObject()
{
	sharedObject::unref( m_sampleBuffer );
}


void BezierSynthOscillatorObject::oscUserDefWaveDblClick()
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

void BezierSynthOscillatorObject::oscUserDefWaveNext()
{
	QString newFile = m_dirScroller.next();
	if ( ! newFile.isEmpty() )
	{
		qWarning("setting wave '%s'", newFile.toStdString().c_str());
		QFileInfo fInfo(PathUtil::toAbsolute(m_sampleBuffer->audioFile()));
		m_sampleBuffer->setAudioFile( PathUtil::toShortestRelative(fInfo.absolutePath() + QDir::separator() + newFile) );
	}
}


void BezierSynthOscillatorObject::oscUserDefWavePrev()
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
	m_volumeLeft = m_volumeModel.value() / 100.0f;
	m_volumeRight = m_volumeModel.value() / 100.0f;
}


void BezierSynthOscillatorObject::updateMutate()
{
	// todo anything here?
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
	m_osc1->m_waveAlgoModel.saveSettings( _doc, _this, "wavealgo" + is );
	m_osc1->m_modulationAlgoModel.saveSettings( _doc, _this, "modalgo" + QString::number( i+1 ) );

	// osc2
	i = 1;
	is = QString::number( i );
	m_osc2->m_volumeModel.saveSettings( _doc, _this, "vol" + is );
	m_osc2->m_coarseModel.saveSettings( _doc, _this, "coarse" + is );
	m_osc2->m_fineModel.saveSettings( _doc, _this, "fine" + is );
	m_osc2->m_waveAlgoModel.saveSettings( _doc, _this, "wavealgo" + is );

	// osc_noise
	i = 2;
	is = QString::number( i );
	m_osc_noise->m_volumeModel.saveSettings( _doc, _this, "vol" + is );
	m_osc_noise->m_coarseModel.saveSettings( _doc, _this, "coarse" + is );
	//m_osc_noise->m_fineModel.saveSettings( _doc, _this, "fine" + is );
	// TODO alway noise?
	m_osc_noise->m_waveAlgoModel.saveSettings( _doc, _this, "wavealgo" + is );
	// TODO alway mix?
	m_osc_noise->m_modulationAlgoModel.saveSettings( _doc, _this, "modalgo" + QString::number( i+1 ) );

	// osc_sample
	i = 3;
	is = QString::number( i );
	m_osc_sample->m_volumeModel.saveSettings( _doc, _this, "vol" + is );
	m_osc_sample->m_coarseModel.saveSettings( _doc, _this, "coarse" + is );
	//m_osc_sample->m_fineModel.saveSettings( _doc, _this, "fine" + is );
	// TODO always sample
	m_osc_sample->m_waveAlgoModel.saveSettings( _doc, _this, "wavealgo" + is );
	// TODO alway mix
	m_osc_sample->m_modulationAlgoModel.saveSettings( _doc, _this, "modalgo" + QString::number( i+1 ) );
	_this.setAttribute( "userwavefile" + is, m_osc_sample->m_sampleBuffer->audioFile() );
}




void BezierSynth::loadSettings( const QDomElement & _this )
{
	int i = 0;
	QString is = QString::number( i );
	m_osc1->m_volumeModel.loadSettings( _this, "vol" + is );
	m_osc1->m_coarseModel.loadSettings( _this, "coarse" + is );
	m_osc1->m_fineModel.loadSettings( _this, "fine" + is );
	m_osc1->m_waveAlgoModel.loadSettings( _this, "wavealgo" + is );
	m_osc1->m_modulationAlgoModel.loadSettings( _this, "modalgo" + QString::number( i+1 ) );
	m_osc1->m_sampleBuffer->setAudioFile( _this.attribute( "userwavefile" + is ) );

	i = 2;
	is = QString::number( i );
	m_osc2->m_volumeModel.loadSettings( _this, "vol" + is );
	m_osc2->m_coarseModel.loadSettings( _this, "coarse" + is );
	m_osc2->m_fineModel.loadSettings( _this, "fine" + is );
	m_osc2->m_waveAlgoModel.loadSettings( _this, "wavealgo" + is );
	m_osc2->m_modulationAlgoModel.loadSettings( _this, "modalgo" + QString::number( i+1 ) );
	m_osc2->m_sampleBuffer->setAudioFile( _this.attribute( "userwavefile" + is ) );

	i = 3;
	is = QString::number( i );
	m_osc_noise->m_volumeModel.loadSettings( _this, "vol" + is );
	m_osc_noise->m_coarseModel.loadSettings( _this, "coarse" + is );
	m_osc_noise->m_fineModel.loadSettings( _this, "fine" + is );
	m_osc_noise->m_waveAlgoModel.loadSettings( _this, "wavealgo" + is );
	m_osc_noise->m_modulationAlgoModel.loadSettings( _this, "modalgo" + QString::number( i+1 ) );
	m_osc_noise->m_sampleBuffer->setAudioFile( _this.attribute( "userwavefile" + is ) );

	i = 4;
	is = QString::number( i );
	m_osc_sample->m_volumeModel.loadSettings( _this, "vol" + is );
	m_osc_sample->m_coarseModel.loadSettings( _this, "coarse" + is );
	m_osc_sample->m_fineModel.loadSettings( _this, "fine" + is );
	m_osc_sample->m_waveAlgoModel.loadSettings( _this, "wavealgo" + is );
	m_osc_sample->m_modulationAlgoModel.loadSettings( _this, "modalgo" + QString::number( i+1 ) );
	m_osc_sample->m_sampleBuffer->setAudioFile( _this.attribute( "userwavefile" + is ) );
}


QString BezierSynth::nodeName() const
{
	return( beziersynth_plugin_descriptor.name );
}


void BezierSynth::playNote( NotePlayHandle * _n, sampleFrame * _working_buffer )
{
	if (!_n->m_pluginData)
	{
		// TODO no need for left and right the do the same math twice
		auto oscs_l = std::array<BezierOsc*, 4>{};
		auto oscs_r = std::array<BezierOsc*, 4>{};

		// osc_sample
		oscs_l[3] = new BezierOsc(
				static_cast<BezierOsc::WaveAlgo>( m_osc_sample->m_waveAlgoModel.value() ),
				static_cast<BezierOsc::ModulationAlgo>( m_osc_sample->m_modulationAlgoModel.value() ),
				_n->frequency(),
				m_osc_sample->m_detuning,
				m_osc_sample->m_volumeLeft,
				// TODO mutate model
				nullptr, // no sub osc
				m_osc_sample->m_sampleBuffer );
		oscs_r[3] = new BezierOsc(
				static_cast<BezierOsc::WaveAlgo>( m_osc_sample->m_waveAlgoModel.value() ),
				static_cast<BezierOsc::ModulationAlgo>( m_osc_sample->m_modulationAlgoModel.value() ),
				_n->frequency(),
				m_osc_sample->m_detuning,
				m_osc_sample->m_volumeRight,
				// TODO mutate model
				nullptr, // no sub osc
				m_osc_sample->m_sampleBuffer );

		// osc_noise
		oscs_l[2] = new BezierOsc(
				static_cast<BezierOsc::WaveAlgo>( m_osc_noise->m_waveAlgoModel.value() ),
				static_cast<BezierOsc::ModulationAlgo>( m_osc_noise->m_modulationAlgoModel.value() ),
				_n->frequency(),
				m_osc_noise->m_detuning,
				m_osc_noise->m_volumeLeft,
				// TODO mutate
				oscs_l[3],
				nullptr );
		oscs_r[2] = new BezierOsc(
				static_cast<BezierOsc::WaveAlgo>( m_osc_noise->m_waveAlgoModel.value() ),
				static_cast<BezierOsc::ModulationAlgo>( m_osc_noise->m_modulationAlgoModel.value() ),
				_n->frequency(),
				m_osc_noise->m_detuning,
				m_osc_noise->m_volumeRight,
				// TODO mutate
				oscs_r[3],
				nullptr );

		// osc2
		oscs_l[1] = new BezierOsc(
				static_cast<BezierOsc::WaveAlgo>( m_osc2->m_waveAlgoModel.value() ),
				static_cast<BezierOsc::ModulationAlgo>( m_osc2->m_modulationAlgoModel.value() ),
				_n->frequency(),
				m_osc2->m_detuning,
				m_osc2->m_volumeLeft,
				// TODO mutate
				oscs_l[2],
				nullptr);
		oscs_r[1] = new BezierOsc(
				static_cast<BezierOsc::WaveAlgo>( m_osc2->m_waveAlgoModel.value() ),
				static_cast<BezierOsc::ModulationAlgo>( m_osc2->m_modulationAlgoModel.value() ),
				_n->frequency(),
				m_osc2->m_detuning,
				m_osc2->m_volumeRight,
				// TODO mutate
				oscs_r[2],
				nullptr);

		// osc1
		oscs_l[0] = new BezierOsc(
				static_cast<BezierOsc::WaveAlgo>( m_osc1->m_waveAlgoModel.value() ),
				static_cast<BezierOsc::ModulationAlgo>( m_osc1->m_modulationAlgoModel.value() ),
				_n->frequency(),
				m_osc1->m_detuning,
				m_osc1->m_volumeLeft,
				// TODO mutate
				oscs_l[1],
				nullptr );
		oscs_r[0] = new BezierOsc(
				static_cast<BezierOsc::WaveAlgo>( m_osc1->m_waveAlgoModel.value() ),
				static_cast<BezierOsc::ModulationAlgo>( m_osc1->m_modulationAlgoModel.value() ),
				_n->frequency(),
				m_osc1->m_detuning,
				m_osc1->m_volumeRight,
				// TODO mutate
				oscs_r[1],
				nullptr );

		_n->m_pluginData = new oscPtr;
		static_cast<oscPtr *>( _n->m_pluginData )->oscLeft = oscs_l[0];
		static_cast<oscPtr *>( _n->m_pluginData )->oscRight = oscs_r[0];
	}

	BezierOsc * osc_l = static_cast<oscPtr *>( _n->m_pluginData )->oscLeft;
	BezierOsc * osc_r = static_cast<oscPtr *>( _n->m_pluginData )->oscRight;

	const fpp_t frames = _n->framesLeftForCurrentPeriod();
	const f_cnt_t offset = _n->noteOffset();

	osc_l->update( _working_buffer + offset, frames, 0 );
	osc_r->update( _working_buffer + offset, frames, 1 );

	applyFadeIn(_working_buffer, _n);
	applyRelease( _working_buffer, _n );

}




void BezierSynth::deleteNotePluginData( NotePlayHandle * _n )
{
	delete static_cast<BezierOsc *>( static_cast<oscPtr *>( _n->m_pluginData )->oscLeft );
	delete static_cast<BezierOsc *>( static_cast<oscPtr *>( _n->m_pluginData )->oscRight );
	delete static_cast<oscPtr *>( _n->m_pluginData );
}




gui::PluginView* BezierSynth::instantiateView( QWidget * _parent )
{
	return new gui::BezierSynthView( this, _parent );
}


void BezierSynth::updateAllDetuning()
{
	m_osc2->updateDetuning();
	m_osc2->updateDetuning();
}


namespace gui
{


class BezierSynthKnob : public Knob
{
public:
	BezierSynthKnob( QWidget * _parent ) : Knob( KnobType::Dark28, _parent )
	{
		setFixedSize( 30, 35 );
	}
};

// 82, 109


BezierSynthView::BezierSynthView( Instrument * _instrument, QWidget * _parent ) :
	InstrumentViewFixedSize( _instrument, _parent )
{
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );

	// modulations buttons, how osc2 modulates osc1

	const int mod_x = 66;
	const int mod1_y = 40;

	auto mix_osc_btn = new PixmapButton(this, nullptr);
	mix_osc_btn->move( mod_x + 0, mod1_y );
	mix_osc_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "mix_active" ) );
	mix_osc_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "mix_inactive" ) );
	mix_osc_btn->setToolTip(tr("Mix output of oscillators 1 & 2"));

	auto am_osc_btn = new PixmapButton(this, nullptr);
	am_osc_btn->move( mod_x + 35, mod1_y );
	am_osc_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "am_active" ) );
	am_osc_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "am_inactive" ) );
	am_osc_btn->setToolTip(tr("Modulate amplitude of oscillator 1 by oscillator 2"));

	auto fm_osc_btn = new PixmapButton(this, nullptr);
	fm_osc_btn->move( mod_x + 70, mod1_y );
	fm_osc_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "fm_active" ) );
	fm_osc_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "fm_inactive" ) );
	fm_osc_btn->setToolTip(tr("Modulate frequency of oscillator 1 by oscillator 2"));

	m_modBtnGrp = new automatableButtonGroup( this );
	m_modBtnGrp->addButton( mix_osc_btn );
	m_modBtnGrp->addButton( am_osc_btn );
	m_modBtnGrp->addButton( fm_osc_btn );

	const int osc_1y = 0;
	const int osc_2y = 53;
	const int osc_3y = 93;
	const int osc_4y = 133;

	int i = 0;
	int knob_y = osc_1y + 10;
	int knob_x = 5;

	// OSCILLATOR 1

	// setup volume-knob
	auto vol1 = new BezierSynthKnob(this);
	vol1->setVolumeKnob( true );
	vol1->move( knob_x, knob_y );
	vol1->setHintText( tr( "Osc %1 volume:" ).arg( i+1 ), "%" );

	// setup coarse-knob
	Knob * course1 = new BezierSynthKnob(this);
	course1->move( knob_x + 40, knob_y );
	course1->setHintText( tr( "Osc %1 coarse detuning:" ).arg( i + 1 ) , " " + tr( "semitones" ) );

	// setup knob for fine-detuning
	Knob * fine1 = new BezierSynthKnob(this);
	fine1->move(  knob_x + 72, knob_y );
	fine1->setHintText( tr( "Osc %1 fine detuning:" ).arg( i + 1 ), " " + tr( "cents" ) );

	// setup mutate-knob
	auto mutate1 = new BezierSynthKnob(this);
	mutate1->setVolumeKnob( true );
	mutate1->move(  knob_x + 104, knob_y );
	mutate1->setHintText( tr( "Osc %1 mutate:" ).arg( i+1 ), "%" );


	int btn_y = 0;
	int btn_x = 135;
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

	auto bezierz_btn1 = new PixmapButton(this, nullptr);
	bezierz_btn1->move( btn_x + ( x++ * 15), btn_y );
	bezierz_btn1->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "bezierz_wave_active" ) );
	bezierz_btn1->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "bezierz_wave_inactive" ) );
	bezierz_btn1->setToolTip(tr("BezierZ wave"));

	auto uwb1 = new PixmapButton(this, nullptr);
	uwb1->move( btn_x + ( x++ * 15), btn_y );
	uwb1->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "usr_shape_active" ) );
	uwb1->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "usr_shape_inactive" ) );
	uwb1->setToolTip(tr("User-defined wave"));

	auto lrn1 = new LeftRightNav(this);
	lrn1->setCursor( QCursor( Qt::PointingHandCursor ) );
	lrn1->move( 215, btn_y );

	auto wabg1 = new automatableButtonGroup(this);

	wabg1->addButton( sin_wave_btn1 );
	wabg1->addButton( noise_btn1 );
	wabg1->addButton( bezierz_btn1 );
	wabg1->addButton( uwb1 );


	m_osc1Knobs = BezierOscKnobs( vol1, course1, fine1, mutate1, wabg1, uwb1, lrn1 );

	// OSCILLATOR 2

	i = 1;
	knob_y = osc_2y + 10;


	// setup volume-knob
	auto vol2 = new BezierSynthKnob(this);
	vol2->setVolumeKnob( true );
	vol2->move( knob_x, knob_y );
	vol2->setHintText( tr( "Osc %1 volume:" ).arg( i+1 ), "%" );

	// setup coarse-knob
	Knob * course2 = new BezierSynthKnob(this);
	course2->move( knob_x + 40, knob_y );
	course2->setHintText( tr( "Osc %1 coarse detuning:" ).arg( i + 1 ) , " " + tr( "semitones" ) );

	// setup knob for fine-detuning
	Knob * fine2 = new BezierSynthKnob(this);
	fine2->move(  knob_x + 72, knob_y );
	fine2->setHintText( tr( "Osc %1 fine detuning:" ).arg( i + 1 ), " " + tr( "cents" ) );

	// setup mutate-knob
	auto mutate2 = new BezierSynthKnob(this);
	mutate2->setVolumeKnob( true );
	mutate2->move(  knob_x + 104, knob_y );
	mutate2->setHintText( tr( "Osc %1 mutate:" ).arg( i+1 ), "%" );


	btn_y = 53;

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

	auto bezierz_btn2 = new PixmapButton(this, nullptr);
	bezierz_btn2->move( btn_x + ( x++ * 15), btn_y );
	bezierz_btn2->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "bezierz_wave_active" ) );
	bezierz_btn2->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "bezierz_wave_inactive" ) );
	bezierz_btn2->setToolTip(tr("BezierZ wave"));

	auto uwb2 = new PixmapButton(this, nullptr);
	uwb2->move( btn_x + ( x++ * 15), btn_y );
	uwb2->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "usr_shape_active" ) );
	uwb2->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "usr_shape_inactive" ) );
	uwb2->setToolTip(tr("User-defined wave"));

	auto lrn2 = new LeftRightNav(this);
	lrn2->setCursor( QCursor( Qt::PointingHandCursor ) );
	lrn2->move( 215, btn_y );

	auto wabg2 = new automatableButtonGroup(this);

	wabg2->addButton( sin_wave_btn2 );
	wabg2->addButton( noise_btn2 );
	wabg2->addButton( bezierz_btn2 );
	wabg2->addButton( uwb2 );

	m_osc2Knobs = BezierOscKnobs( vol2, course2, fine2, mutate2, wabg2, uwb2, lrn2 );


	// NOISE

	i = 2;
	knob_y = osc_3y + 10;

	// setup volume-knob
	auto vol3 = new BezierSynthKnob(this);
	vol3->setVolumeKnob( true );
	vol3->move( knob_x, knob_y );
	vol3->setHintText( tr( "Osc %1 volume:" ).arg( i+1 ), "%" );

	// setup coarse-knob
	Knob * course3 = new BezierSynthKnob(this);
	course3->move( knob_x + 40, knob_y );
	course3->setHintText( tr( "Osc %1 coarse detuning:" ).arg( i + 1 ) , " " + tr( "semitones" ) );

	m_oscNoiseKnobs = BezierOscKnobs( vol3, course3, nullptr, nullptr, nullptr, nullptr, nullptr );


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

	btn_y = 133;
	x = 0;

	// TODO default clicks

	auto uwb4 = new PixmapButton(this, nullptr);
	uwb4->move( btn_x + ( x++ * 15), btn_y );
	uwb4->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "usr_shape_active" ) );
	uwb4->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "usr_shape_inactive" ) );
	uwb4->setToolTip(tr("User-defined wave"));

	auto lrn4 = new LeftRightNav(this);
	lrn4->setCursor( QCursor( Qt::PointingHandCursor ) );
	lrn4->move( 215, btn_y );

	auto wabg4 = new automatableButtonGroup(this);

	wabg4->addButton( uwb4 );

	m_oscSampleKnobs = BezierOscKnobs( vol4, course4, nullptr, nullptr, wabg4, uwb4, lrn4 );

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
	m_osc1Knobs.m_waveAlgoBtnGrp->setModel( &t->m_osc1->m_waveAlgoModel );

	connect( m_osc1Knobs.m_userWaveButton, SIGNAL( doubleClicked() ), t->m_osc1, SLOT( oscUserDefWaveDblClick() ) );
	connect( m_osc1Knobs.m_userWaveSwitcher, SIGNAL( onNavLeft() ),   t->m_osc1, SLOT( oscUserDefWavePrev() ));
	connect( m_osc1Knobs.m_userWaveSwitcher, SIGNAL( onNavRight() ),  t->m_osc1, SLOT( oscUserDefWaveNext() ));

	// osc2
	m_osc2Knobs.m_volKnob->setModel( &t->m_osc2->m_volumeModel );
	m_osc2Knobs.m_coarseKnob->setModel( &t->m_osc2->m_coarseModel );
	m_osc2Knobs.m_fineKnob->setModel( &t->m_osc2->m_fineModel );
	m_osc2Knobs.m_mutateKnob->setModel( &t->m_osc2->m_mutateModel );
	m_osc2Knobs.m_waveAlgoBtnGrp->setModel( &t->m_osc2->m_waveAlgoModel );

	connect( m_osc2Knobs.m_userWaveButton, SIGNAL( doubleClicked() ), t->m_osc2, SLOT( oscUserDefWaveDblClick() ) );
	connect( m_osc2Knobs.m_userWaveSwitcher, SIGNAL( onNavLeft() ),   t->m_osc2, SLOT( oscUserDefWavePrev() ));
	connect( m_osc2Knobs.m_userWaveSwitcher, SIGNAL( onNavRight() ),  t->m_osc2, SLOT( oscUserDefWaveNext() ));

	// noise
	m_oscNoiseKnobs.m_volKnob->setModel( &t->m_osc_noise->m_volumeModel );
	m_oscNoiseKnobs.m_coarseKnob->setModel( &t->m_osc_noise->m_coarseModel );

	// sample
	m_oscSampleKnobs.m_volKnob->setModel( &t->m_osc_sample->m_volumeModel );
	m_oscSampleKnobs.m_coarseKnob->setModel( &t->m_osc_sample->m_coarseModel );

	connect( m_oscSampleKnobs.m_userWaveButton, SIGNAL( doubleClicked() ), t->m_osc_sample, SLOT( oscUserDefWaveDblClick() ) );
	connect( m_oscSampleKnobs.m_userWaveSwitcher, SIGNAL( onNavLeft() ),   t->m_osc_sample, SLOT( oscUserDefWavePrev() ));
	connect( m_oscSampleKnobs.m_userWaveSwitcher, SIGNAL( onNavRight() ),  t->m_osc_sample, SLOT( oscUserDefWaveNext() ));

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
