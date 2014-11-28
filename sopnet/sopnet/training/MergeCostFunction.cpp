#include <pipeline/Process.h>
#include <util/Logger.h>
#include <util/ProgramOptions.h>
#include <sopnet/exceptions.h>
#include <sopnet/neurons/NeuronExtractor.h>
#include <sopnet/segments/EndSegment.h>
#include <sopnet/segments/ContinuationSegment.h>
#include <sopnet/segments/BranchSegment.h>
#include "MergeCostFunction.h"

static logger::LogChannel linearcostfunctionlog("linearcostfunctionlog", "[MergeCostFunction] ");

util::ProgramOption optionCorrectlyMergedPairReward(
		util::_module           = "sopnet.training.gold_standard",
		util::_long_name        = "correctlyMergedPairReward",
		util::_description_text = "The reward in the gold-standard search objective for each correctly merged pixel pair (according to the ground truth). This number should be negative to be a reward.",
		util::_default_value    = -1);

util::ProgramOption optionIncorrectlyMergedThreshold(
		util::_module           = "sopnet.training.gold_standard",
		util::_long_name        = "incorrectlyMergedThreshold",
		util::_description_text = "The number of incorrectly merged pixels (not pairs) per segment, after which the segment is considered a flase merge and the falseMergeCosts apply in the gold-standard search objective.",
		util::_default_value    = 100);

util::ProgramOption optionFalseMergeCosts(
		util::_module           = "sopnet.training.gold_standard",
		util::_long_name        = "falseMergeCosts",
		util::_description_text = "The costs in the gold-standard search objective for segments that have been identified as false merges.",
		util::_default_value    = 1e6 /* a million */);

MergeCostFunction::MergeCostFunction() :
	_costFunction(new costs_function_type(boost::bind(&MergeCostFunction::costs, this, _1, _2, _3, _4))),
	_correctlyMergedPairReward(optionCorrectlyMergedPairReward),
	_incorrectlyMergedThreshold(optionIncorrectlyMergedThreshold),
	_falseMergeCosts(optionFalseMergeCosts) {

	registerInput(_groundTruth, "ground truth");
	registerOutput(_costFunction, "cost function");
}

void
MergeCostFunction::updateOutputs() {}

void
MergeCostFunction::costs(
		const std::vector<boost::shared_ptr<EndSegment> >&          ends,
		const std::vector<boost::shared_ptr<ContinuationSegment> >& continuations,
		const std::vector<boost::shared_ptr<BranchSegment> >&       branches,
		std::vector<double>& segmentCosts) {

	unsigned int i = 0;

	foreach (boost::shared_ptr<EndSegment> end, ends) {

		double c = costs(*end);

		segmentCosts[i] += c;
		i++;
	}

	foreach (boost::shared_ptr<ContinuationSegment> continuation, continuations) {

		// prefer continuations a little over 2 times end
		double c = costs(*continuation) - 0.5;

		segmentCosts[i] += c;
		i++;
	}

	foreach (boost::shared_ptr<BranchSegment> branch, branches) {

		// prefer branches a little over 3 times end
		double c = costs(*branch) - 0.5;

		segmentCosts[i] += c;
		i++;
	}
}

double
MergeCostFunction::costs(const Segment& segment) {

	// count all gt label occurences covered by this segment
	getGtLabels(segment);

	// find the largest covered ground truth region and the sum of ground-truth 
	// pixels covered

	unsigned int maxOverlap = 0;
	unsigned int sumGtPixels = 0;

	float k;
	unsigned int overlap;

	foreach (boost::tie(k, overlap), _gtLabels) {

		if (overlap > maxOverlap) {

			maxOverlap = overlap;
		}

		sumGtPixels += overlap;
	}

	unsigned int correctlyMerged   = maxOverlap;
	unsigned int incorrectlyMerged = sumGtPixels - maxOverlap;

	if (incorrectlyMerged > _incorrectlyMergedThreshold)
		return _falseMergeCosts;

	return _correctlyMergedPairReward*correctlyMerged*correctlyMerged;
}

void
MergeCostFunction::getGtLabels(const Segment& segment) {

	_gtLabels.clear();

	foreach (boost::shared_ptr<Slice> slice, segment.getSlices()) {

		unsigned int section = slice->getSection();

		foreach (const util::point<unsigned int>& p, slice->getComponent()->getPixels()) {

			float gtLabel = (*(*_groundTruth)[section])(p.x, p.y);

			// ignore the background label
			if (gtLabel > 0)
				_gtLabels[gtLabel]++;
		}
	}
}

int
MergeCostFunction::segmentSize(const Segment& segment) {

	const std::vector<boost::shared_ptr<Slice> >& slices = segment.getSlices();

	unsigned int sum = 0;

	foreach (boost::shared_ptr<Slice> slice, slices)
		sum += slice->getComponent()->getSize();

	return sum;
}


