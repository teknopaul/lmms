
#include <QLabel>
#include <QWidget>

#include "Groove.h"

#include "lmms_basics.h"
#include "Note.h"
#include "Song.h"
#include "HydrogenSwing.h"
#include "HalfSwing.h"
#include "MidiSwing.h"
#include "GrooveExperiments.h"

namespace lmms
{


Groove::Groove()
{
}

Groove::~Groove()
{
}

/**
 * Default groove is no groove. Not even a wiggle.
 */
void Groove::apply( Note * _n )
{
	_n->setNoteOffset(0);
}


void Groove::saveSettings( QDomDocument & _doc, QDomElement & _element )
{

}

void Groove::loadSettings( const QDomElement & _this )
{

}

QWidget * Groove::instantiateView( QWidget * _parent )
{
	return new lmms::gui::GrooveText(this, "No groove", _parent);
}

Groove * Groove::instantiateGroove(QString type, QObject * _parent)
{
	if ( type == HydrogenSwing::name() )
	{
		 return new HydrogenSwing( _parent );
	}
	if ( type == MidiSwing::name() )
	{
		 return new MidiSwing( _parent );
	}
	if ( type == HalfSwing::name() )
	{
		 return new HalfSwing( _parent );
	}
	if ( type == GrooveExperiments::name() )
	{
		 return new GrooveExperiments( _parent );
	}
	return new Groove();
}

}


namespace lmms::gui
{

/**
 * Groove view that is just text
 */
GrooveText::GrooveText(Groove * _groove, const QString text, QWidget * _parent) : QLabel(text,  _parent )
{
	setAlignment(Qt::AlignTop | Qt::AlignLeft);
}

GrooveText::~GrooveText()
{
}

}

