#include <sstream>
#include "FeaturesView.h"

FeaturesView::FeaturesView() {

	registerInput(_segments, "segments");
	registerInput(_features, "features");
	registerInput(_problemConfiguration, "problem configuration");
	registerInput(_objective, "objective");
	registerOutput(_painter, "painter");
}

void
FeaturesView::updateOutputs() {

	std::stringstream featuresString;

	foreach (boost::shared_ptr<EndSegment> end, _segments->getEnds())
		appendFeatures(featuresString, end->getId());
	foreach (boost::shared_ptr<ContinuationSegment> continuation, _segments->getContinuations())
		appendFeatures(featuresString, continuation->getId());
	foreach (boost::shared_ptr<BranchSegment> branch, _segments->getBranches())
		appendFeatures(featuresString, branch->getId());

	_painter->setText(featuresString.str());
	_painter->setTextSize(10.0);
}

void
FeaturesView::appendFeatures(std::stringstream& stream, unsigned int segmentId) {

	const unsigned int variable = _problemConfiguration->getVariable(segmentId);
	const double costs = _objective->getCoefficients()[variable];

	stream << "costs: " << costs << "; ";

	const std::vector<double>&      features = _features->get(segmentId);
	const std::vector<std::string>& names    = _features->getNames();

	for (unsigned int i = 0; i < features.size(); i++) {

		if (i < names.size())
			stream << names[i] << ": ";

		stream << features[i] << ",  ";
	}
}
