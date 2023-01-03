/*
 * TempoSyncKnob.h - adds bpm to ms conversion for knob class
 *
 * Copyright (c) 2005-2008 Danny McRae <khjklujn/at/yahoo.com>
 * Copyright (c) 2009-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_GUI_TEMPO_SYNC_KNOB_H
#define LMMS_GUI_TEMPO_SYNC_KNOB_H

#include <QPixmap>
#include <QPointer>

#include "Knob.h"
#include "TempoSyncKnobModel.h"

namespace lmms::gui
{

class MeterDialog;

class LMMS_EXPORT TempoSyncKnob : public Knob
{
	Q_OBJECT
public:
	TempoSyncKnob( knobTypes knobNum, QWidget* parent = nullptr, const QString& name = QString() );
	~TempoSyncKnob() override;

	const QString & syncDescription();
	void setSyncDescription( const QString & _new_description );

	const QPixmap & syncIcon();
	void setSyncIcon( const QPixmap & _new_pix );

	TempoSyncKnobModel * model()
	{
		return castModel<TempoSyncKnobModel>();
	}

	void modelChanged() override;


signals:
	void syncDescriptionChanged( const QString & _new_description );
	void syncIconChanged();


protected:
	void contextMenuEvent( QContextMenuEvent * _me ) override;


protected slots:
	void updateDescAndIcon();
	void showCustom();


private:
	QPixmap m_tempoSyncIcon;
	QString m_tempoSyncDescription;

	QPointer<MeterDialog> m_custom;

} ;



} // namespace lmms::gui

#endif
