#include <util/Logger.h>
#include <util/point.hpp>
#include <imageprocessing/ConnectedComponent.h>
#include <sopnet/segments/EndSegment.h>
#include <sopnet/segments/ContinuationSegment.h>
#include <sopnet/segments/BranchSegment.h>
#include "RandomForestCostFunction.h"

static logger::LogChannel randomforestcostfunctionlog("randomforestcostfunctionlog", "[RandomForestCostFunction] ");

RandomForestCostFunction::RandomForestCostFunction() :
	_costFunction(boost::bind(&RandomForestCostFunction::costs, this, _1)) {

	registerInput(_features, "features");
	registerInput(_randomForest, "random forest");
	registerOutput(_costFunction, "cost function");
}

void
RandomForestCostFunction::updateOutputs() {

	// nothing to do, here
}

double
RandomForestCostFunction::costs(const Segment& segment) {

	LOG_ALL(randomforestcostfunctionlog) << "evaluating segment " << segment.getId() << std::endl;

	double prob = _randomForest->getProbabilities(_features->get(segment.getId()))[1];

	double costs = -log(std::max(0.001, std::min(0.999, prob)));

	LOG_ALL(randomforestcostfunctionlog) << "final costs are " << costs << std::endl;

	return costs;
}

