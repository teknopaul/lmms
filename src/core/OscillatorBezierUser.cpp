
#include <cmath>
#include <algorithm>
#include <QStringList>
#include <QRegularExpression>
#include <QFile>
#include <QDomDocument>
#include <QDebug>
#include <QMessageBox>
#include "ConfigManager.h"
#include "FileDialog.h"
#include "OscillatorBezierBase.h"
#include "OscillatorBezierUser.h"
#include "PathUtil.h"

namespace lmms
{

/**
 * Reusable specification of a OscillatorBezier loaded from a file.
 * File is laoded and parsed once, many Oscs are made from it, (one per note play)
 *
 * TODO plenty of opportunity for the user to crash LMMS here.
 * TODO sane limits at least
 * TODO guards on all array indexing
 * TODO guards on sanity of the modulation numbers
 */
OscillatorBezierDefinition::OscillatorBezierDefinition() :
	m_number_of_segments(6),
	m_mod_count(0),
	m_name("user wave")
{
	std::memset(m_segments, 0, sizeof(m_segments));
	std::memset(m_modulation_defs, 0, sizeof(m_modulation_defs));
}

static inline float limitX(float x) {
	if (x < 0.0f) return 0.0f;
	if (x > 1.0f) return 1.0f;
	return x;
}
static inline float limitY(float y) {
	if (y < -1.0f) return -1.0f;
	if (y > 1.0f) return 1.0f;
	return y;
}
/**
 * SVG template is a 1 by 1 grid, which Inkscape prefers
 * audio is 1 by 2 (y is -1.0 to 1.0)
 */
static inline float nomalizeY(float y) {
	float n = (y - 0.5f) * 2.0f;
	return n;
}
static inline Point nomalize(Point p, int i) {
	p.y = nomalizeY(p.y);
	// points cannot be outside the grid, but bezier handles can
	if (i == 0) {
		p.x = limitX(p.x);
		p.y = limitY(p.y);
	}
	qDebug("svg normalized %f,%f", p.x, p.y);
	return p;
}

static inline void showError(const char * message)
{
	QMessageBox::critical(nullptr, "Error", message);
}

/**
 * parse the audio <path id="wave" d="...">
 */
bool OscillatorBezierDefinition::parseBezierPath(const QString &d)
{
	// 1. Tokenize on whitespace
	QStringList tokens = d.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
	if (tokens.isEmpty()) {
		showError("Empty path string in SVG");
		return false;
	}

	int idx = 0;

	// 2. Expect "M x,y"
	if (tokens[idx] != "M" && tokens[idx] != "m") {
		showError("Path must start with 'M'");
		return false;
	}
	idx++;

	QStringList startCoords = tokens[idx++].split(",");
	if (startCoords.size() != 2) {
		showError("Invalid M coordinates");
		return false;
	}

	if ( !qFuzzyCompare(startCoords[0].toFloat(), 0.0f) ||
		 !qFuzzyCompare(nomalizeY(startCoords[1].toFloat()), 0.0f) ) {
		showError("Invalid M coordinates, waves must start at 0,0");
		return false;
	}

	// set to exact values despite fuzzy compare above
	m_segments[0][0].x = 0.0f;
	m_segments[0][0].y = 0.0f;

	if (tokens[idx++] != "C" && tokens[idx++] != "c") {
		showError("Curve must be cubic / bezier (start with 'C')");
		return false;
	}

	// 3. Parse one bezier path (aka segment)
	int segmentIdx = -1;
	while (idx < tokens.size()) {
		segmentIdx++;
		if (segmentIdx > 5) {
			return false;
		}

		// SVG has 3 points
		if (idx + 2 >= tokens.size()) {
			showError("Incomplete 'C' command, needs 3 points");
			return false;
		}
		// m_segments has 4, start point is end point of previous curve
		if (segmentIdx > 0) {
			m_segments[segmentIdx][0] = m_segments[segmentIdx -1][3];
		}
		// Each curve has exactly 3 coordinate pairs
		for (int i = 1; i < 4; ++i) {

			QStringList coords = tokens[idx++].split(",");
			if (coords.size() != 2) {
				showError("Invalid C coordinate, points should have x,y");
				return false;
			}
			Point p { coords[0].toFloat(), coords[1].toFloat() };

			m_segments[segmentIdx][i] = nomalize(p, i);
		}
	}
	m_number_of_segments = segmentIdx + 1;

	// 4. Validate final point == (1.0, 0.0)
	Point last = m_segments[segmentIdx][3];
	if (qFuzzyCompare(last.x, 1.0f) && qFuzzyCompare(last.y , 0.0f)) {
		qDebug("loaded %i segments ", m_number_of_segments);
		return true;
	} else {
		showError("Wave must end at 0,0");
		return false;
	}
}

static inline void warn(int idx) {
	qWarning("bad modulation point? %d", idx);
}

static inline bool outOfRangeX(float x) {
	return x < 0.0f || x > 1.0f;
}
static inline bool outOfRangeY(float y) {
	return y < -1.0f || y > 1.0f;
}
static inline bool saneCheck(ModulationDef * modDef, int idx)
{
	if (modDef->x) {
		if (outOfRangeX(modDef->start)) warn(idx);
		if (outOfRangeX(modDef->start + modDef->range)) warn(idx);
	} else {
		if (outOfRangeY(modDef->start)) warn(idx);
		if (outOfRangeY(modDef->start + modDef->range)) warn(idx);
	}
	// points MUST be inside range, at the start and after modulation by max range
	if (modDef->item == 0)
	{
		if (modDef->x) {
			if (outOfRangeX(modDef->start)) return false;
			if (outOfRangeX(modDef->start + modDef->range)) return false;
		} else {
			if (outOfRangeY(modDef->start)) return false;
			if (outOfRangeY(modDef->start + modDef->range)) return false;
		}
	}
	return true;
}
/**
 * parse the instructions for the mutation / modulation
 * range=-0.1    +- is the direction in X forward or back
 * point=0.1.x   this creates a pointer to m_Segments[0][1].x
 * range=+0.1
 * point=1.2.x
 * ...
 *
 * User has to ensure it makes sense.
 * TODO validate points wont go out of range
 * TODO (advanced) validate the whole curve stays in range
 *
 * One know controls up to 6 point mutations
 *
 * @return true if sane
 */
bool OscillatorBezierDefinition::parseModulations(const QString &desc)
{
	// 1. Tokenize on whitespace
	QStringList tokens = desc.split('\n', Qt::SkipEmptyParts);
	if (tokens.isEmpty()) {
		showError("Empty mutations string in SVG");
		// TODO default mutations
		return true;
	}
	int lineNumber = 0;
	ModulationDef * modNext = nullptr;
	m_mod_count = 0;

	while (lineNumber < tokens.size()) {
		QString line = tokens[lineNumber++];
		if (line.trimmed().size() == 0) continue;
		if (line[0] == "#") continue;

		QStringList mutation = line.split("=");
		if (mutation.size() != 2) {
			qWarning("mods should be range|point=value");
			m_mod_count = 0;
			if (modNext) delete modNext;
			return false;
		}
		if (mutation[0] == "range") {
			modNext = new ModulationDef();
			modNext->range = mutation[1].toFloat();
		}
		else if (mutation[0] == "point") {
			if (modNext == nullptr)
			{
				qWarning("specify range for each and every point");
				m_mod_count = 0;
				return false;
			}
			QStringList point = mutation[1].split(".");
			if (point.size() == 3) {
				modNext->segment = point[0].toInt();
				modNext->item = point[1].toInt();
				modNext->x = point[2] == "x";
				if (modNext->segment > m_number_of_segments) {
					qWarning("no segment %i", modNext->segment);
					m_mod_count = 0;
					delete modNext;
					return false;
				}
				if (modNext->item > 3) {
					qWarning("insane item %i", modNext->item);
					m_mod_count = 0;
					delete modNext;
					return false;
				}
				// start point loaded from SVG as drawn
				Point p = m_segments[modNext->segment][modNext->item];
				modNext->start = modNext->x ? p.x : p.y;
				if ( saneCheck(modNext, m_mod_count) )
				{
					m_modulation_defs[m_mod_count++] = modNext;
				}
				modNext = nullptr;
				if (m_mod_count == MAX_MODULATIONS) return true;
			} else {
				qWarning("invalid mutation point (define range first)");
			}
		} else {
			// TODO c string for the line that won't parse
			qWarning("unparseable line @ %d", lineNumber);
		}
	}
	qWarning("parse modulations: %i " , m_mod_count);
	return true;
}

/**
 * Parse specific SGV format, with identified paths for audio.
 */
int OscillatorBezierDefinition::loadFromSVG(QString path)
{
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly)) {
		qWarning() << "Could not open SVG file: " << path;
		showError("Could not open file");
		return -1;
	}

	QDomDocument doc;
	if (!doc.setContent(&file)) {
		showError("Could not parse XML content");
		return -2;
	}
	QDomElement svg = doc.documentElement();

	QDomNodeList tspans = svg.elementsByTagName("tspan");
	for (int i = 0; i < tspans.count(); ++i) {
		QDomElement tspan = tspans.at(i).toElement();
		if (tspan.attribute("id") == "wavename") {
			m_name = tspan.text();
			qDebug() << "svg sound name: " << m_name;
		}
	}
	QDomNodeList paths = svg.elementsByTagName("path");
	for (int i = 0; i < paths.count(); ++i) {
		QDomElement path = paths.at(i).toElement();
		if (path.attribute("id") == "wave") {
			QString d = path.attribute("d");
			qDebug() << "Found 'd' sound wave path";
			if ( parseBezierPath(d) ) {
				// Find <g id="audiolayer">
				QDomNodeList groups = svg.elementsByTagName("g");
				for (int i = 0; i < groups.count(); ++i) {
					QDomElement g = groups.at(i).toElement();
					if (g.attribute("id") == "audiolayer") {
						// Get <desc>
						QDomNodeList descNodes = g.elementsByTagName("desc");
						if (!descNodes.isEmpty()) {
							QDomElement desc = descNodes.at(0).toElement();
							QString descText = desc.text();
							qDebug() << "Desc text:\n" << descText;
							parseModulations(descText);
						}
					}
				}
				return 0;
			}
		}
	}

	return -3;
}



QString OscillatorBezierDefinition::openSvgFile()
{
	gui::FileDialog ofd(nullptr, "Open svg file");

	QString dir;
	if (!m_svgFile.isEmpty())
	{
		QString f = m_svgFile;
		if (QFileInfo(f).isRelative())
		{
			f = ConfigManager::inst()->userSamplesDir() + f;
			if (QFileInfo(f).exists() == false)
			{
				f = ConfigManager::inst()->factorySamplesDir() +
								m_svgFile;
			}
		}
		dir = QFileInfo(f).absolutePath();
	}
	else
	{
		dir = ConfigManager::inst()->userSamplesDir();
	}
	// change dir to position of previously opened file
	ofd.setDirectory(dir);
	ofd.setFileMode(gui::FileDialog::ExistingFiles);

	// set filters
	QStringList types;
	types << "Svg Wave-Files (*.wave.svg)";
	ofd.setNameFilters(types);
	if (!m_svgFile.isEmpty())
	{
		// select previously opened file
		ofd.selectFile(QFileInfo(m_svgFile).fileName());
	}

	if (ofd.exec () == QDialog::Accepted)
	{
		if (ofd.selectedFiles().isEmpty())
		{
			return QString();
		}
		if ( loadFromSVG(ofd.selectedFiles()[0]) == 0 ) {
			return PathUtil::toShortestRelative(ofd.selectedFiles()[0]);
		}
	}

	return QString();
}


/**
 * A bezier oscillator made from data loaded from and SVG file generated in Inkscape
 */
OscillatorBezierUser::OscillatorBezierUser(OscillatorBezierDefinition * oscDef, float mod) :
	OscillatorBezierBase(),
	m_next_mod(-1.0f)
{
	overrideNumOfSegment(oscDef->m_number_of_segments);
	overrideSegments(oscDef->m_segments);
	initModulations(oscDef);
	if (mod >= 0.0f && mod <= 1.0f ) {
		modulate(mod);
		applyModulations();
	}
}

/**
 * copies modulation def but changing the value to be changed to a point to the
 * m_segments array,
 */
void OscillatorBezierUser::initModulations(OscillatorBezierDefinition * oscDef)
{
	for (int i = 0; i < oscDef->m_mod_count; i++ ) {
		ModulationDef * modDef = oscDef->m_modulation_defs[i];
		Point * toMod = &m_segments[modDef->segment][modDef->item];
		m_modulations[i] = {
			modDef->range,
			modDef->start,
			modDef->x ? &toMod->x : &toMod->y
		};
	}
	m_mod_count = oscDef->m_mod_count;
}

void OscillatorBezierUser::modulate(float mod)
{
	m_next_mod = mod;
}

void OscillatorBezierUser::applyModulations()
{
	if (m_next_mod < 0.0f) return;

	for (int i = 0; i < m_mod_count; i++ ) {
		*m_modulations[i].coord = m_modulations[i].start + (m_next_mod * m_modulations[i].range);
	}
}

} // namespace lmms
