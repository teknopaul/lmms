/*
 * XoneControlDialog.h - control dialog for xone effect
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

#ifndef XONE_CONTROL_DIALOG_H
#define XONE_CONTROL_DIALOG_H

#include "EffectControlDialog.h"

namespace lmms
{

class XoneControls;


namespace gui
{

class XoneControlDialog : public EffectControlDialog
{
	Q_OBJECT
public:
	XoneControlDialog( XoneControls* controls );
	~XoneControlDialog() override = default;

} ;


} // namespace gui

} // namespace lmms

#endif
