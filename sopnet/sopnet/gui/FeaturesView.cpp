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

	unsigned int segmentId = 0;

	// get the id of the last (usually single) segment in _segments
	foreach (boost::shared_ptr<EndSegment> end, _segments->getEnds())
		segmentId = end->getId();
	foreach (boost::shared_ptr<ContinuationSegment> continuation, _segments->getContinuations())
		segmentId = continuation->getId();
	foreach (boost::shared_ptr<BranchSegment> branch, _segments->getBranches())
		segmentId = branch->getId();

	const unsigned int variable = _problemConfiguration->getVariable(segmentId);

	_painter->setCosts(_objective->getCoefficients()[variable]);
	_painter->setFeatures(_features->get(segmentId));
	_painter->setFeatureNames(_features->getNames());
}

