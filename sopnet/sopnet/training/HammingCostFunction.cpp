#include <pipeline/Process.h>
#include <util/Logger.h>
#include <sopnet/exceptions.h>
#include <sopnet/segments/EndSegment.h>
#include <sopnet/segments/ContinuationSegment.h>
#include <sopnet/segments/BranchSegment.h>
#include "HammingCostFunction.h"

static logger::LogChannel linearcostfunctionlog("linearcostfunctionlog", "[HammingCostFunction] ");

HammingCostFunction::HammingCostFunction() :
	_costFunction(new costs_function_type(boost::bind(&HammingCostFunction::costs, this, _1, _2, _3, _4))) {

	registerInput(_goldStandard, "gold standard");
	registerOutput(_costFunction, "cost function");
}

void
HammingCostFunction::updateOutputs() {}

void
HammingCostFunction::costs(
		const std::vector<boost::shared_ptr<EndSegment> >&          ends,
		const std::vector<boost::shared_ptr<ContinuationSegment> >& continuations,
		const std::vector<boost::shared_ptr<BranchSegment> >&       branches,
		std::vector<double>& segmentCosts) {

	unsigned int i = 0;

	foreach (boost::shared_ptr<EndSegment> end, ends) {

		double c = costs(end);

		segmentCosts[i] += c;
		i++;
	}

	foreach (boost::shared_ptr<ContinuationSegment> continuation, continuations) {

		double c = costs(continuation);

		segmentCosts[i] += c;
		i++;
	}

	foreach (boost::shared_ptr<BranchSegment> branch, branches) {

		double c = costs(branch);

		segmentCosts[i] += c;
		i++;
	}
}

// It would be nicer if there was a Continuations::contains(boost::shared_ptr<Segments> segment).
// Then we could just call that in a generic function, like the one below.

//double
//HammingCostFunction::costs(boost::shared_ptr<Segment> segment) {
//
//	// if the segment is contained in the gold standard the cost should be -1
//	// and +1 if it is not. The constant offset that would be needed here to exactly
//	// match the hamming distance is ignored here as it won't change the minimum of the objective.
//
//	_goldStandard->contains(segment) ? return -1.0 : return 1.0;
//     
//	// or:
//
//	//if (_goldStandard->contains(segment)) {
//	//	return -1.0;
//	//} 
//	//else {
//	//	return 1.0;
//	//}
//}

double
HammingCostFunction::costs(boost::shared_ptr<EndSegment> end) {


	if (_goldStandard->contains(end)) {
		return -1.0;
	} 
	else {
		return 1.0;
	}

}

double
HammingCostFunction::costs(boost::shared_ptr<ContinuationSegment> continuation) {

	if (_goldStandard->contains(continuation)) {
		return -1.0;
	} 
	else {
		return 1.0;
	}

}

double
HammingCostFunction::costs(boost::shared_ptr<BranchSegment> branch) {

	if (_goldStandard->contains(branch)) {
		return -1.0;
	} 
	else {
		return 1.0;
	}

}
