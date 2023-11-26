
#include <QLabel>
#include <QWidget>
#include <QVBoxLayout>

#include "Groove.h"

#include "lmms_basics.h"
#include "Note.h"
#include "Song.h"

namespace lmms
{


Groove::Groove()
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

