#include <util/Logger.h>
#include <util/point.hpp>
#include <imageprocessing/ConnectedComponent.h>
#include "SegmentEvaluator.h"
#include "EndSegment.h"
#include "ContinuationSegment.h"
#include "BranchSegment.h"

static logger::LogChannel segmentevaluatorlog("segmentevaluatorlog", "[SegmentEvaluator] ");

SegmentEvaluator::SegmentEvaluator() :
	_costFunction(boost::bind(&SegmentEvaluator::costs, this, _1, boost::make_shared<SegmentCostFunctionParameters>())) {

	registerInput(_parameters, "parameters");
	registerOutput(_costFunction, "cost function");
}

void
SegmentEvaluator::updateOutputs() {

	*_costFunction = boost::bind(&SegmentEvaluator::costs, this, _1, boost::make_shared<SegmentCostFunctionParameters>(*_parameters));
}

double
SegmentEvaluator::costs(const Segment& segment, boost::shared_ptr<SegmentCostFunctionParameters> parameters) {

	LOG_ALL(segmentevaluatorlog) << "evaluating segment " << segment.getId() << std::endl;

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
SegmentEvaluator::getCosts(const EndSegment& end) {

	return sqrt(end.getSlice()->getComponent()->getSize())*0.01;
}

double
SegmentEvaluator::getCosts(const ContinuationSegment& continuation) {

	boost::shared_ptr<ConnectedComponent> source = continuation.getSourceSlice()->getComponent();
	boost::shared_ptr<ConnectedComponent> target = continuation.getTargetSlice()->getComponent();

	const util::point<double>& sourceCenter = source->getCenter();
	const util::point<double>& targetCenter = target->getCenter();

	LOG_ALL(segmentevaluatorlog) << "connects from " << sourceCenter << " to " << targetCenter << std::endl;

	double diffX = sourceCenter.x - targetCenter.x;
	double diffY = sourceCenter.y - targetCenter.y;

	double distance = diffX*diffX + diffY*diffY;

	LOG_ALL(segmentevaluatorlog) << "distance is " << distance << std::endl;

	return distance;
}

double
SegmentEvaluator::getCosts(const BranchSegment& branch) {

	boost::shared_ptr<ConnectedComponent> source  = branch.getSourceSlice()->getComponent();
	boost::shared_ptr<ConnectedComponent> target1 = branch.getTargetSlice1()->getComponent();
	boost::shared_ptr<ConnectedComponent> target2 = branch.getTargetSlice2()->getComponent();

	const util::point<double>& sourceCenter  = source->getCenter();
	const util::point<double>& targetCenter1 = target1->getCenter();
	const util::point<double>& targetCenter2 = target2->getCenter();

	LOG_ALL(segmentevaluatorlog) << "connects from " << sourceCenter << " to " << targetCenter1 << " and " << targetCenter2 << std::endl;

	double diffX = sourceCenter.x - 0.5*(targetCenter1.x + targetCenter2.x);
	double diffY = sourceCenter.y - 0.5*(targetCenter1.y + targetCenter2.y);

	double distance = diffX*diffX + diffY*diffY;

	LOG_ALL(segmentevaluatorlog) << "distance is " << distance << std::endl;

	return distance;
}

