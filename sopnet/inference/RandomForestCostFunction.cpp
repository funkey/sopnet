#include <limits>

#include <util/Logger.h>
#include <util/point.hpp>
#include <imageprocessing/ConnectedComponent.h>
#include <sopnet/segments/EndSegment.h>
#include <sopnet/segments/ContinuationSegment.h>
#include <sopnet/segments/BranchSegment.h>
#include "RandomForestCostFunction.h"

static logger::LogChannel randomforestcostfunctionlog("randomforestcostfunctionlog", "[RandomForestCostFunction] ");

util::ProgramOption optionMinSegmentProbability(
		util::_module           = "sopnet.inference",
		util::_long_name        = "minSegmentProbability",
		util::_description_text = "The minimal probability a segments needs to have (according to the RF-classifier) to accept it.",
		util::_default_value    = "0.05");

RandomForestCostFunction::RandomForestCostFunction() :
	_costFunction(boost::bind(&RandomForestCostFunction::costs, this, _1, _2, _3, _4)),
	_maxSegmentCosts(-std::log(optionMinSegmentProbability.as<double>())) {

	registerInput(_features, "features");
	registerInput(_randomForest, "random forest");
	registerOutput(_costFunction, "cost function");
}

void
RandomForestCostFunction::updateOutputs() {

	// invalidate cache
	_cache.clear();
}

void
RandomForestCostFunction::costs(
		const std::vector<boost::shared_ptr<EndSegment> >&          ends,
		const std::vector<boost::shared_ptr<ContinuationSegment> >& continuations,
		const std::vector<boost::shared_ptr<BranchSegment> >&       branches,
		std::vector<double>& segmentCosts) {

	segmentCosts.resize(ends.size() + continuations.size() + branches.size(), 0);

	if (segmentCosts.size() == _cache.size()) {

		for (unsigned int i = 0; i < segmentCosts.size(); i++)
			segmentCosts[i] += _cache[i];

		return;
	}

	_cache.resize(ends.size() + continuations.size() + branches.size());

	unsigned int i = 0;

	foreach (boost::shared_ptr<EndSegment> end, ends) {

		double c = costs(*end);

		segmentCosts[i] += c;
		_cache[i] = c;

		i++;
	}

	foreach (boost::shared_ptr<ContinuationSegment> continuation, continuations) {

		double c = costs(*continuation);

		if (c >= _maxSegmentCosts)
			c = std::numeric_limits<double>::infinity();

		segmentCosts[i] += c;
		_cache[i] = c;

		i++;
	}

	foreach (boost::shared_ptr<BranchSegment> branch, branches) {

		double c = costs(*branch);

		if (c >= _maxSegmentCosts)
			c = std::numeric_limits<double>::infinity();

		segmentCosts[i] += c;
		_cache[i] = c;

		i++;
	}
}

double
RandomForestCostFunction::costs(const Segment& segment) {

	double prob = _randomForest->getProbabilities(_features->get(segment.getId()))[1];

	// [23.02, 0.0]
	return -log(std::max(1e-10, prob));
}

