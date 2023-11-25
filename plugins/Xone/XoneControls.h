/*
 * XoneControls.h - controls for Xone effect
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

#ifndef AMPLIFIER_CONTROLS_H
#define AMPLIFIER_CONTROLS_H

#include "EffectControls.h"
#include "XoneControlDialog.h"

namespace lmms
{

class XoneEffect;

namespace gui
{
class XoneControlDialog;
}


class XoneControls : public EffectControls
{
	Q_OBJECT
public:
	XoneControls( XoneEffect* effect );
	~XoneControls() override = default;

	void saveSettings( QDomDocument & _doc, QDomElement & _parent ) override;
	void loadSettings( const QDomElement & _this ) override;
	inline QString nodeName() const override
	{
		return "XoneControls";
	}

	int controlCount() override
	{
		return 2;
	}

	gui::EffectControlDialog* createView() override
	{
		return new gui::XoneControlDialog( this );
	}


private slots:
	void changeControl();

private:
	XoneEffect* m_effect;
	FloatModel m_freqModel;
	FloatModel m_wildModel;

	friend class gui::XoneControlDialog;
	friend class XoneEffect;

} ;


} // namespace lmms

#endif
