#include "PriorCostFunction.h"
#include <imageprocessing/ConnectedComponent.h>
#include <sopnet/segments/EndSegment.h>
#include <sopnet/segments/ContinuationSegment.h>
#include <sopnet/segments/BranchSegment.h>
#include <sopnet/slices/Slice.h>

logger::LogChannel priorcostfunctionlog("priorcostfunctionlog", "[PriorCostFunction] ");

PriorCostFunction::PriorCostFunction() :
	_costFunction(new costs_function_type(boost::bind(&PriorCostFunction::costs, this, _1, _2, _3, _4))) {

	registerInput(_parameters, "parameters");

	registerOutput(_costFunction, "cost function");
}

void
PriorCostFunction::updateOutputs() {

	LOG_DEBUG(priorcostfunctionlog) << "\"updating\" output..." << std::endl;
	// nothing to do here
}

void
PriorCostFunction::costs(
		const std::vector<boost::shared_ptr<EndSegment> >&          ends,
		const std::vector<boost::shared_ptr<ContinuationSegment> >& continuations,
		const std::vector<boost::shared_ptr<BranchSegment> >&       branches,
		std::vector<double>& segmentCosts) {

	segmentCosts.resize(ends.size() + continuations.size() + branches.size(), 0);

	double priorEndCosts          = _parameters->priorEnd;
	double priorContinuationCosts = _parameters->priorContinuation;
	double priorBranchCosts       = _parameters->priorBranch;

	LOG_DEBUG(priorcostfunctionlog)
			<< "setting priors, end " << priorEndCosts
			<< " continuation " << priorContinuationCosts
			<< " branch " << priorBranchCosts << std::endl;

	unsigned int i = 0;

	unsigned int numSections = 0;
	foreach (boost::shared_ptr<EndSegment> end, ends)
		numSections = std::max(numSections, end->getInterSectionInterval());

	foreach (boost::shared_ptr<EndSegment> end, ends) {

		// end segments out of the block are for free
		if (end->getInterSectionInterval() != 0 && end->getInterSectionInterval() != numSections)
			segmentCosts[i] += priorEndCosts;

		i++;
	}

	foreach (boost::shared_ptr<ContinuationSegment> continuation, continuations) {

		segmentCosts[i] += priorContinuationCosts;
		i++;
	}

	foreach (boost::shared_ptr<BranchSegment> branch, branches) {

		segmentCosts[i] += priorBranchCosts;
		i++;
	}
}

