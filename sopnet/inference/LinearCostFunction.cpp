#include <util/Logger.h>
#include <util/point.hpp>
#include <imageprocessing/ConnectedComponent.h>
#include <sopnet/segments/EndSegment.h>
#include <sopnet/segments/ContinuationSegment.h>
#include <sopnet/segments/BranchSegment.h>
#include "LinearCostFunction.h"

static logger::LogChannel linearcostfunctionlog("linearcostfunctionlog", "[LinearCostFunction] ");

LinearCostFunction::LinearCostFunction() :
	_costFunction(boost::bind(&LinearCostFunction::costs, this, _1, boost::make_shared<LinearCostFunctionParameters>())) {

	registerInput(_parameters, "parameters");
	registerOutput(_costFunction, "cost function");
}

void
LinearCostFunction::updateOutputs() {

	*_costFunction = boost::bind(&LinearCostFunction::costs, this, _1, boost::make_shared<LinearCostFunctionParameters>(*_parameters));
}

double
LinearCostFunction::costs(const Segment& segment, boost::shared_ptr<LinearCostFunctionParameters> parameters) {

	LOG_ALL(linearcostfunctionlog) << "evaluating segment " << segment.getId() << std::endl;

	/**
	 * Yes, dynamic casts are evil. This is not a good solution in terms of
	 * performance. It would be better to have a cost function for each segment
	 * type. But since noone really uses this cost function here anyway, we can
	 * postpone the good implementation and live with this.
	 */

	if (const EndSegment* end = dynamic_cast<const EndSegment*>(&segment))
		return getCosts(*end);

	if (const ContinuationSegment* continuation = dynamic_cast<const ContinuationSegment*>(&segment))
		return getCosts(*continuation);

	if (const BranchSegment* branch = dynamic_cast<const BranchSegment*>(&segment))
		return getCosts(*branch);

	return 0;
}

double
LinearCostFunction::getCosts(const EndSegment& end) {

	return sqrt(end.getSlice()->getComponent()->getSize())*0.01;
}

double
LinearCostFunction::getCosts(const ContinuationSegment& continuation) {

	boost::shared_ptr<ConnectedComponent> source = continuation.getSourceSlice()->getComponent();
	boost::shared_ptr<ConnectedComponent> target = continuation.getTargetSlice()->getComponent();

	const util::point<double>& sourceCenter = source->getCenter();
	const util::point<double>& targetCenter = target->getCenter();

	LOG_ALL(linearcostfunctionlog) << "connects from " << sourceCenter << " to " << targetCenter << std::endl;

	double diffX = sourceCenter.x - targetCenter.x;
	double diffY = sourceCenter.y - targetCenter.y;

	double distance = diffX*diffX + diffY*diffY;

	LOG_ALL(linearcostfunctionlog) << "distance is " << distance << std::endl;

	return distance;
}

double
LinearCostFunction::getCosts(const BranchSegment& branch) {

	boost::shared_ptr<ConnectedComponent> source  = branch.getSourceSlice()->getComponent();
	boost::shared_ptr<ConnectedComponent> target1 = branch.getTargetSlice1()->getComponent();
	boost::shared_ptr<ConnectedComponent> target2 = branch.getTargetSlice2()->getComponent();

	const util::point<double>& sourceCenter  = source->getCenter();
	const util::point<double>& targetCenter1 = target1->getCenter();
	const util::point<double>& targetCenter2 = target2->getCenter();

	LOG_ALL(linearcostfunctionlog) << "connects from " << sourceCenter << " to " << targetCenter1 << " and " << targetCenter2 << std::endl;

	double diffX = sourceCenter.x - 0.5*(targetCenter1.x + targetCenter2.x);
	double diffY = sourceCenter.y - 0.5*(targetCenter1.y + targetCenter2.y);

	double distance = diffX*diffX + diffY*diffY;

	LOG_ALL(linearcostfunctionlog) << "distance is " << distance << std::endl;

	return distance;
}

