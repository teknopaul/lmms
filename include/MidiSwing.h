#ifndef MIDISWING_H
#define MIDISWING_H

#include <QtCore/QObject>

#include "Groove.h"
#include "lmms_basics.h"
#include "Note.h"

namespace lmms {

/**
 * A swing groove that adjusts by whole ticks.
 * Someone might like it, also might be able to save the output to a midi file later.
 */
class MidiSwing : public QObject, public Groove
{
	Q_OBJECT
public:
	MidiSwing(QObject * _parent=0 );
	~MidiSwing();

	void apply( Note * _n ) override;
	int applyMidiSwing(int _pos_in_eigth);

	void loadSettings( const QDomElement & _this ) override;
	void saveSettings( QDomDocument & _doc, QDomElement & _element ) override;
	inline virtual QString nodeName() const override
	{
		return "midi";
	}

	QWidget * instantiateView( QWidget * _parent ) override;

};

}

#endif // MIDISWING_H
