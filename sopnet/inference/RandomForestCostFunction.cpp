#include <util/Logger.h>
#include <util/point.hpp>
#include <imageprocessing/ConnectedComponent.h>
#include <sopnet/segments/EndSegment.h>
#include <sopnet/segments/ContinuationSegment.h>
#include <sopnet/segments/BranchSegment.h>
#include "RandomForestCostFunction.h"

static logger::LogChannel randomforestcostfunctionlog("randomforestcostfunctionlog", "[RandomForestCostFunction] ");

RandomForestCostFunction::RandomForestCostFunction() :
	_costFunction(boost::bind(&RandomForestCostFunction::costs, this, _1, _2, _3, _4)) {

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

	unsigned int numSections = 0;
	foreach (boost::shared_ptr<EndSegment> end, ends)
		numSections = std::max(numSections, end->getInterSectionInterval());

	foreach (boost::shared_ptr<EndSegment> end, ends) {

		double c;

		// end segments out of the block are for free
		if (end->getInterSectionInterval() == 0 || end->getInterSectionInterval() == numSections)
			c = 0.0;
		else
			c = costs(*end);

		segmentCosts[i] += c;
		_cache[i] = c;

		i++;
	}

	foreach (boost::shared_ptr<ContinuationSegment> continuation, continuations) {

		segmentCosts[i] += costs(*continuation);
		_cache[i] = costs(*continuation);

		i++;
	}

	foreach (boost::shared_ptr<BranchSegment> branch, branches) {

		segmentCosts[i] += costs(*branch);
		_cache[i] = costs(*branch);

		i++;
	}
}

double
RandomForestCostFunction::costs(const Segment& segment) {

	double prob = _randomForest->getProbabilities(_features->get(segment.getId()))[1];

	return -log(std::max(0.001, std::min(0.999, prob)));
}

