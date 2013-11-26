#include <util/foreach.h>
#include "FeaturesPainter.h"

FeaturesPainter::FeaturesPainter(unsigned int numFeatures) :
	_cellX(200.0),
	_cellY(20.0),
	_textPaintersDirty(true),
	_segmentId(0) {

	setSize(0, 0, 4*_cellX, (numFeatures+1)*_cellY);
}

void
FeaturesPainter::setCosts(double costs) {

	_costs = costs;
	_textPaintersDirty = true;
}

void
FeaturesPainter::setFeatures(const std::vector<double>& features) {

	_features = features;
	_textPaintersDirty = true;

	setSize(-_cellX/10.0, 0, 2*_cellX, (_features.size() + 2)*_cellY);
}

void
FeaturesPainter::setFeatureNames(const std::vector<std::string>& names) {

	_featureNames = names;
	_textPaintersDirty = true;
}

void
FeaturesPainter::setSegmentId(unsigned int segmentId) {

	_segmentId = segmentId;
}

void
FeaturesPainter::setGroundTruthScore(boost::shared_ptr<std::map<unsigned int, double> > groundTruthScore) {

	_groundTruthScore = groundTruthScore;
	_textPaintersDirty = true;
}

bool
FeaturesPainter::draw(const util::rect<double>& roi, const util::point<double>& resolution) {

	if (_textPaintersDirty)
		updateTextPainters();

	boost::shared_ptr<gui::TextPainter> painter;
	util::point<int> position;

	foreach (boost::tie(painter, position), _textPainters) {

		double x = position.x;
		double y = position.y;

		x *= _cellX;
		y *= _cellY;

		glTranslatef( x,  y, 0.0f);
		painter->draw(roi - util::point<double>(x, y), resolution);
		glTranslatef(-x, -y, 0.0f);
	}

	return false;
}

void
FeaturesPainter::updateTextPainters() {

	_textPainters.clear();

	// costs
	_textPainters.push_back(
			std::make_pair(
					createTextPainter("costs:"),
					util::point<int>(0, 0)
			));
	_textPainters.push_back(
			std::make_pair(
					createTextPainter(_costs),
					util::point<int>(1, 0)
			));

	// names
	for (int i = 0; i < _featureNames.size(); i++)
		_textPainters.push_back(
				std::make_pair(
						createTextPainter(_featureNames[i]),
						util::point<int>(0, i+2)
				));

	// values
	for (int i = 0; i < _features.size(); i++)
		_textPainters.push_back(
				std::make_pair(
						createTextPainter(_features[i]),
						util::point<int>(1, i+2)
				));

	// ground truth score
	if (_groundTruthScore) {

		_textPainters.push_back(
				std::make_pair(
						createTextPainter("GT score:"),
						util::point<int>(0, (int)_features.size() + 2)
				));
		_textPainters.push_back(
				std::make_pair(
						createTextPainter((*_groundTruthScore)[_segmentId]),
						util::point<int>(1, (int)_features.size() + 2)
				));
	}

	_textPaintersDirty = false;
}
