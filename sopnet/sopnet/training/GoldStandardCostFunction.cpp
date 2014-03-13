#include <util/Logger.h>
#include <sopnet/exceptions.h>
#include <sopnet/segments/EndSegment.h>
#include <sopnet/segments/ContinuationSegment.h>
#include <sopnet/segments/BranchSegment.h>
#include "GoldStandardCostFunction.h"

static logger::LogChannel linearcostfunctionlog("linearcostfunctionlog", "[GoldStandardCostFunction] ");

GoldStandardCostFunction::GoldStandardCostFunction() :
	_costFunction(new costs_function_type(boost::bind(&GoldStandardCostFunction::costs, this, _1, _2, _3, _4))) {

	registerInput(_groundTruth, "ground truth");
	registerOutput(_costFunction, "cost function");
}

void
GoldStandardCostFunction::updateOutputs() {}

void
GoldStandardCostFunction::costs(
		const std::vector<boost::shared_ptr<EndSegment> >&          ends,
		const std::vector<boost::shared_ptr<ContinuationSegment> >& continuations,
		const std::vector<boost::shared_ptr<BranchSegment> >&       branches,
		std::vector<double>& segmentCosts) {

}

