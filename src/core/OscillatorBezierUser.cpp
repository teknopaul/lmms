
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
 */
OscillatorBezierDefinition::OscillatorBezierDefinition() :
	m_number_of_segments(6),
	m_mod_count(0)
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
	return (y - 0.5) * 2.0f;
}
static inline Point nomalize(Point p, int i) {
	p.y = nomalizeY(p.y);
	// points cannot be outside the grid, but bezier handles can
	if (i == 0) {
		p.x = limitX(p.x);
		p.y = limitY(p.y);
	}
	return p;
}

static inline void showError(const char * message)
{
	QMessageBox::critical(nullptr, "Error", message);
}

/**
 * parse the audio <path id="wave" d="...">
 * TODO error reporting to the user
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
	if (tokens[idx++] != "M") {
		showError("Path must start with 'M'");
		return false;
	}
	QStringList startCoords = tokens[idx++].split(",");
	if (startCoords.size() != 2) {
		showError("Invalid M coordinates");
		return false;
	}

	if ( qFuzzyCompare(startCoords[0].toFloat(), 0.0f) ||
		 qFuzzyCompare(nomalizeY(startCoords[1].toFloat()), 0.0f) ) {
		showError("Invalid M coordinates, waves must start and end at 0,0");
		return false;
	}

	// set to exact values despite fuzzy compare above
	m_segments[0][0].x = 0.0f;
	m_segments[0][0].y = 0.0f;

	if (tokens[idx++] != "C") {
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

	// 4. Validate final point == (1.0, 0.0)
	Point last = m_segments[segmentIdx][3];
	if (qFuzzyCompare(last.x, 1.0f) && qFuzzyCompare(last.y , 0.0f)) {
		return true;
	} else {
		showError("Wave must end at 0,0");
		return false;
	}
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
				// TODO parse modulations from C like string to a function
			}

			// Get <path>
			QDomNodeList paths = g.elementsByTagName("path");
			for (int j = 0; j < paths.count(); ++j) {
				QDomElement path = paths.at(j).toElement();
				if (path.attribute("id") == "wave") {
					QString d = path.attribute("d");
					qDebug() << "Found 'd' sound wave path";
					if ( parseBezierPath(d) ) {
						return 0;
					}
				}
			}
		}
	}
	return 0;
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
OscillatorBezierUser::OscillatorBezierUser(OscillatorBezierDefinition * oscDef) :
	OscillatorBezierBase()
{
	overrideNumOfSegment(oscDef->m_number_of_segments);
	overrideSegments(oscDef->m_segments);
	loadModulations(oscDef);
}

void OscillatorBezierUser::loadModulations(OscillatorBezierDefinition * oscDef)
{
	for (int i = 0; i < oscDef->m_mod_count; i++ ) {
		auto modDef = oscDef->m_modulation_defs[i];
		Point toMod = m_segments[modDef.segment][modDef.item];
		m_modulations[i] = {
			modDef.range,
			modDef.start,
			modDef.x ? &toMod.x : &toMod.y
		};
	}
	m_mod_count = oscDef->m_mod_count;
}

void OscillatorBezierUser::modulate(float mod)
{
	m_mod = mod;
}

void OscillatorBezierUser::applyModulations()
{
	if (m_mod != m_last_mod) {
		for (int i = 0; i < m_mod_count; i++ ) {
			*m_modulations[i].coord = m_modulations[i].start + (m_mod * m_modulations[i].range);
		}
	}
}

} // namespace lmms
