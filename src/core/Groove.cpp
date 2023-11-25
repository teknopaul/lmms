
#include <QtCore/QObject> // needed for midi_tim.h to compile?!?!
#include <QLabel>

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
	return new QLabel("No groove");
}

}
