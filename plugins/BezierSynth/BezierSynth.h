/*
 * _BEZIER_SYNTH_H.h - Audio synthesis using Bezier curve math.
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

#ifndef _BEZIER_SYNTH_H
#define _BEZIER_SYNTH_H

#include <QLabel>

#include "Instrument.h"
#include "InstrumentView.h"
#include "AutomatableModel.h"
#include "LedCheckBox.h"
#include "LeftRightNav.h"
#include "DirectoryScroller.h"
#include "BezierOsc.h"
#include "OscillatorBezierUser.h"


namespace lmms
{


class NotePlayHandle;
class SampleBuffer;
class BezierOsc;


namespace gui
{
class automatableButtonGroup;
class Knob;
class PixmapButton;
class BezierSynthView;
} // namespace gui


class BezierSynthOscillatorObject : public Model
{
	MM_OPERATORS
	Q_OBJECT
public:
	BezierSynthOscillatorObject( Model * _parent, int _idx );
	~BezierSynthOscillatorObject() override;

	float getVolume() {
		return m_volume;
	}

private:
	int m_oscIndex;
	FloatModel m_volumeModel;
	FloatModel m_mutateModel;
	FloatModel m_coarseModel;
	FloatModel m_fineModel;
	FloatModel m_attackModel;
	IntModel m_waveAlgoModel;
	IntModel m_modulationAlgoModel; // only osc1
	SampleBuffer * m_sampleBuffer;  // only oscSample
	BoolModel m_playModel;
	OscillatorBezierDefinition * m_bezierDefinition;
	QString *m_waveName;

	float m_volume;

	// normalized detuning -> x/sampleRate
	float m_detuning;
	DirectoryScroller m_dirScroller;

	friend class BezierSynth;
	friend class gui::BezierSynthView;

private slots:
	void oscUserDefWaveDblClick();
	void oscUserDefWaveNext();
	void oscUserDefWavePrev();
	void oscUserDefSampleDblClick();
	void oscUserDefSampleNext();
	void oscUserDefSamplePrev();

	void updateVolume();
	void updateMutate();
	void updateAttack();
	void updateDetuning();
} ;




class BezierSynth : public Instrument
{
	Q_OBJECT
public:
	BezierSynth( InstrumentTrack * track );
	~BezierSynth() override = default;

	void playNote( NotePlayHandle * n, sampleFrame * working_buffer ) override;
	void playSample( NotePlayHandle * n, sampleFrame * working_buffer, SampleBuffer * sampleBuffer );
	void deleteNotePluginData( NotePlayHandle * _n ) override;

	void saveSettings( QDomDocument & doc, QDomElement & parent ) override;
	void loadSettings( const QDomElement & _this ) override;

	QString nodeName() const override;

	f_cnt_t desiredReleaseFrames() const override
	{
		return( 128 );
	}

	gui::PluginView* instantiateView( QWidget * parent ) override;

protected slots:
	void updateAllDetuning();


private:
	BezierSynthOscillatorObject * m_osc1;
	BezierSynthOscillatorObject * m_osc2;
	BezierSynthOscillatorObject * m_osc_noise;
	BezierSynthOscillatorObject * m_osc_sample;

	using handleState = SampleBuffer::handleState;
	struct oscPtr
	{
		MM_OPERATORS
		BezierOsc * osc;
		handleState * playState;
	};


	friend class gui::BezierSynthView;

} ;


namespace gui
{


class BezierSynthView : public InstrumentViewFixedSize
{
	Q_OBJECT
public:
	BezierSynthView( Instrument * instrument, QWidget * parent );
	~BezierSynthView() override = default;


private:
	void modelChanged() override;

	automatableButtonGroup * m_modBtnGrp;

	struct BezierOscKnobs
	{
		MM_OPERATORS
		BezierOscKnobs( Knob * vol,
					Knob * coarse,
					Knob * fine,
					Knob * mutate,
					Knob * attack,
					LedCheckBox * play,
					automatableButtonGroup * waveAlgoBtnGroup,
					PixmapButton * userWaveButton,
					LeftRightNav * leftRightNav) :
			m_volKnob( vol ),
			m_coarseKnob( coarse ),
			m_fineKnob( fine ),
			m_mutateKnob( mutate ),
			m_attackKnob( attack ),
			m_playLed( play ),
			m_waveAlgoBtnGrp( waveAlgoBtnGroup ),
			m_userWaveButton( userWaveButton ),
			m_userWaveSwitcher( leftRightNav )
		{
		}
		BezierOscKnobs() = default;
		Knob * m_volKnob;
		Knob * m_coarseKnob;
		Knob * m_fineKnob;
		Knob * m_mutateKnob;
		Knob * m_attackKnob;
		LedCheckBox * m_playLed;
		automatableButtonGroup * m_waveAlgoBtnGrp;
		PixmapButton * m_userWaveButton;
		LeftRightNav * m_userWaveSwitcher;

	} ;

	BezierOscKnobs m_osc1Knobs;
	BezierOscKnobs m_osc2Knobs;
	BezierOscKnobs m_oscNoiseKnobs;
	BezierOscKnobs m_oscSampleKnobs;
	QLabel m_osc1WaveName;
	QLabel m_osc2WaveName;
};


} // namespace gui

} // namespace lmms

#endif // _BEZIER_SYNTH_H
