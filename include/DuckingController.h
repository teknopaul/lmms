/*
 * LfoController.h - A LFO-based controller and dialog
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

#ifndef LMMS_DUCKING_CONTROLLER_H
#define LMMS_DUCKING_CONTROLLER_H

#include <QWidget>

#include "Model.h"
#include "AutomatableModel.h"
#include "Controller.h"
#include "ControllerDialog.h"
#include "TempoSyncKnobModel.h"
#include "Oscillator.h"
#include "OscillatorBezier.h"

namespace lmms
{

namespace gui
{

class automatableButtonGroup;
class LedCheckBox;
class PixmapButton;
class Knob;
class TempoSyncKnob;

class DuckingControllerDialog;

}


class DuckingController : public Controller
{
	Q_OBJECT
public:

	enum class DuckShape
	{
		Sine,
		Triangle,
		Saw,
		Square,
		BezierU,
		UserDefined,
		Count //!< Number of all available wave shapes
	};
	constexpr static auto NumDuckShapes = static_cast<std::size_t>(DuckShape::Count);

	DuckingController( Model * _parent );

	~DuckingController() override;

	void saveSettings( QDomDocument & _doc, QDomElement & _this ) override;
	void loadSettings( const QDomElement & _this ) override;
	QString nodeName() const override;


public slots:
	gui::ControllerDialog * createDialog( QWidget * _parent ) override;


protected:
	// The internal per-controller value updating function
	void updateValueBuffer() override;

	FloatModel m_baseModel;
	TempoSyncKnobModel m_speedModel;
	FloatModel m_amountModel;
	FloatModel m_phaseModel;
	IntModel m_waveModel;
	IntModel m_multiplierModel;

	float m_duration;
	float m_phaseOffset;
	float m_currentPhase;

	sample_t (*m_sampleFunction)( const float );

private:
	float m_heldSample;
	SampleBuffer * m_userDefSampleBuffer;
	OscillatorBezier * m_oscillatorBezier;

protected slots:
	void updatePhase();
	void updateSampleFunction();
	void updateDuration();

	friend class gui::DuckingControllerDialog;

} ;


namespace gui
{

class DuckingControllerDialog : public ControllerDialog
{
	Q_OBJECT
public:
	DuckingControllerDialog( Controller * _controller, QWidget * _parent );
	~DuckingControllerDialog() override;


protected:
	void contextMenuEvent( QContextMenuEvent * _me ) override;
	void modelChanged() override;

	DuckingController * m_ducking;

	Knob * m_baseKnob;
	TempoSyncKnob * m_speedKnob;
	Knob * m_amountKnob;
	Knob * m_phaseKnob;
	PixmapButton * m_userLfoBtn;
	automatableButtonGroup * m_waveBtnGrp;
	automatableButtonGroup * m_multiplierBtnGrp;


private:
	PixmapButton * m_userWaveBtn;

private slots:
	void askUserDefWave();

} ;


} // namespace gui

} // namespace lmms

#endif // LMMS_DUCKING_CONTROLLER_H
