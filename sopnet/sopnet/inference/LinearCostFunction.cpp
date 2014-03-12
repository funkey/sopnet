#include <util/Logger.h>
#include <util/point.hpp>
#include <imageprocessing/ConnectedComponent.h>
#include <sopnet/exceptions.h>
#include <sopnet/segments/EndSegment.h>
#include <sopnet/segments/ContinuationSegment.h>
#include <sopnet/segments/BranchSegment.h>
#include "LinearCostFunction.h"

static logger::LogChannel linearcostfunctionlog("linearcostfunctionlog", "[LinearCostFunction] ");

LinearCostFunction::LinearCostFunction() :
	_costFunction(new costs_function_type(boost::bind(&LinearCostFunction::costs, this, _1, _2, _3, _4))) {

	registerInput(_features, "features");
	registerInput(_parameters, "parameters");
	registerOutput(_costFunction, "cost function");
}

void
LinearCostFunction::updateOutputs() {

	// invalidate cache
	_cache.clear();

	if (_features->numFeatures() != _parameters->getWeights().size()) {

		std::stringstream message;
		message
				<< "number of features (" << _features->numFeatures()
				<< ") does not match number of weights (" << _parameters->getWeights().size()
				<< ")";
		BOOST_THROW_EXCEPTION(InvalidParameters()
				<< error_message(message.str())
				<< STACK_TRACE);
	}
}

void
LinearCostFunction::costs(
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

	const std::vector<double> weights = _parameters->getWeights();

	unsigned int i = 0;

	foreach (boost::shared_ptr<EndSegment> end, ends) {

		double c = costs(*end, weights);

		segmentCosts[i] += c;
		_cache[i] = c;

		i++;
	}

	foreach (boost::shared_ptr<ContinuationSegment> continuation, continuations) {

		double c = costs(*continuation, weights);

		segmentCosts[i] += c;
		_cache[i] = c;

		i++;
	}

	foreach (boost::shared_ptr<BranchSegment> branch, branches) {

		double c = costs(*branch, weights);

		segmentCosts[i] += c;
		_cache[i] = c;

		i++;
	}
}

double
LinearCostFunction::costs(const Segment& segment, const std::vector<double>& weights) {

	const std::vector<double>& features = _features->get(segment.getId());

	double costs = 0;
	for (unsigned int i = 0; i < features.size(); i++)
		costs += features[i]*weights[i];

	return costs;
}
