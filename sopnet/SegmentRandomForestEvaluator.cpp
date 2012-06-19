#include <util/Logger.h>
#include <util/point.hpp>
#include <imageprocessing/ConnectedComponent.h>
#include "SegmentRandomForestEvaluator.h"
#include "EndSegment.h"
#include "ContinuationSegment.h"
#include "BranchSegment.h"

static logger::LogChannel segmentevaluatorlog("segmentevaluatorlog", "[SegmentRandomForestEvaluator] ");

SegmentRandomForestEvaluator::SegmentRandomForestEvaluator() :
	_costFunction(boost::bind(&SegmentRandomForestEvaluator::costs, this, _1)) {

	registerInput(_features, "features");
	registerInput(_randomForest, "random forest");
	registerOutput(_costFunction, "cost function");
}

void
SegmentRandomForestEvaluator::updateOutputs() {

	// nothing to do, here
}

double
SegmentRandomForestEvaluator::costs(const Segment& segment) {

	LOG_ALL(segmentevaluatorlog) << "evaluating segment " << segment.getId() << std::endl;

	double prob = _randomForest->getProbabilities(_features->get(segment.getId()))[1];

	double costs = -log(std::max(0.001, std::min(0.999, prob)));

	LOG_ALL(segmentevaluatorlog) << "final costs are " << costs << std::endl;

	return costs;
}

