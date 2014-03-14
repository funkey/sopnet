#include <fstream>

#include <boost/filesystem.hpp>

#include <util/helpers.hpp>
#include <imageprocessing/ConnectedComponent.h>
#include <sopnet/exceptions.h>
#include <sopnet/segments/EndSegment.h>
#include <sopnet/segments/ContinuationSegment.h>
#include <sopnet/segments/BranchSegment.h>
#include "GeometryFeatureExtractor.h"

logger::LogChannel geometryfeatureextractorlog("geometryfeatureextractorlog", "[GeometryFeatureExtractor] ");

util::ProgramOption optionDisableSliceDistanceFeature(
		util::_module           = "sopnet.features",
		util::_long_name        = "disableSliceDistanceFeature",
		util::_description_text = "Disable the use of slice distance features.");

GeometryFeatureExtractor::GeometryFeatureExtractor() :
	_features(new Features()),
	_overlap(false, false),
	_alignedOverlap(false, true),
	_noSliceDistance(optionDisableSliceDistanceFeature) {

	registerInput(_segments, "segments");
	registerOutput(_features, "features");
}

void
GeometryFeatureExtractor::updateOutputs() {

	LOG_DEBUG(geometryfeatureextractorlog) << "extracting features" << std::endl;

	_features->clear();

	if (_noSliceDistance)
		_features->resize(_segments->size(), 11);
	else
		_features->resize(_segments->size(), 15);

	// features for end segments
	_features->addName("e size");

	// features for continuation and branch
	_features->addName("c&b center distance");
	_features->addName("c&b set difference");
	_features->addName("c&b set difference ratio");
	_features->addName("c&b aligned set difference");
	_features->addName("c&b aligned set difference ratio");
	_features->addName("c&b size");
	_features->addName("c&b overlap");
	_features->addName("c&b overlap ratio");
	_features->addName("c&b aligned overlap");
	_features->addName("c&b aligned overlap ratio");

	if (!_noSliceDistance) {

		_features->addName("c&b average slice distance");
		_features->addName("c&b max slice distance");
		_features->addName("c&b aligned average slice distance");
		_features->addName("c&b aligned max slice distance");
	}

	foreach (boost::shared_ptr<EndSegment> segment, _segments->getEnds())
		getFeatures(*segment);

	foreach (boost::shared_ptr<ContinuationSegment> segment, _segments->getContinuations())
		getFeatures(*segment);

	foreach (boost::shared_ptr<BranchSegment> segment, _segments->getBranches())
		getFeatures(*segment);

	LOG_ALL(geometryfeatureextractorlog) << "found features: " << *_features << std::endl;

	LOG_DEBUG(geometryfeatureextractorlog) << "done" << std::endl;

	// free memory
	_distance.clearCache();
}

template <typename SegmentType>
void
GeometryFeatureExtractor::getFeatures(const SegmentType& segment) {

	computeFeatures(segment, _features->get(segment.getId()));
}

void
GeometryFeatureExtractor::computeFeatures(const EndSegment& end, std::vector<double>& features) {

	features[0] = end.getSlice()->getComponent()->getSize();
	features[1] = Features::NoFeatureValue;
	features[2] = Features::NoFeatureValue;
	features[3] = Features::NoFeatureValue;
	features[4] = Features::NoFeatureValue;
	features[5] = Features::NoFeatureValue;
	features[6] = Features::NoFeatureValue;
	features[7] = Features::NoFeatureValue;
	features[8] = Features::NoFeatureValue;
	features[9] = Features::NoFeatureValue;
	features[10] = Features::NoFeatureValue;

	if (!_noSliceDistance) {

		features[11] = Features::NoFeatureValue;
		features[12] = Features::NoFeatureValue;
		features[13] = Features::NoFeatureValue;
		features[14] = Features::NoFeatureValue;
	}
}

void
GeometryFeatureExtractor::computeFeatures(const ContinuationSegment& continuation, std::vector<double>& features) {

	const util::point<double>& sourceCenter = continuation.getSourceSlice()->getComponent()->getCenter();
	const util::point<double>& targetCenter = continuation.getTargetSlice()->getComponent()->getCenter();

	unsigned int sourceSize = continuation.getSourceSlice()->getComponent()->getSize();
	unsigned int targetSize = continuation.getTargetSlice()->getComponent()->getSize();

	util::point<double> difference = sourceCenter - targetCenter;

	double distance = difference.x*difference.x + difference.y*difference.y;

	double overlap = _overlap(*continuation.getSourceSlice(), *continuation.getTargetSlice());

	double overlapRatio = overlap/(sourceSize + targetSize - overlap);

	double alignedOverlap = _alignedOverlap(*continuation.getSourceSlice(), *continuation.getTargetSlice());

	double alignedOverlapRatio = alignedOverlap/(sourceSize + targetSize - overlap);

	double setDifference = (sourceSize - overlap) + (targetSize - overlap);

	double setDifferenceRatio = setDifference/(sourceSize + targetSize);

	double alignedSetDifference = (sourceSize - alignedOverlap) + (targetSize - alignedOverlap);

	double alignedSetDifferenceRatio = alignedSetDifference/(sourceSize + targetSize);

	features[0] = Features::NoFeatureValue;
	features[1] = distance;
	features[2] = setDifference;
	features[3] = setDifferenceRatio;
	features[4] = alignedSetDifference;
	features[5] = alignedSetDifferenceRatio;
	features[6] =
			(continuation.getSourceSlice()->getComponent()->getSize() +
			 continuation.getTargetSlice()->getComponent()->getSize())*0.5;
	features[7] = overlap;
	features[8] = overlapRatio;
	features[0] = alignedOverlap;
	features[10] = alignedOverlapRatio;

	if (!_noSliceDistance) {

		double averageSliceDistance, maxSliceDistance;

		_distance(*continuation.getSourceSlice(), *continuation.getTargetSlice(), true, false, averageSliceDistance, maxSliceDistance);

		double alignedAverageSliceDistance, alignedMaxSliceDistance;

		_distance(*continuation.getSourceSlice(), *continuation.getTargetSlice(), true, true, alignedAverageSliceDistance, alignedMaxSliceDistance);

		features[11] = averageSliceDistance;
		features[12] = maxSliceDistance;
		features[13] = alignedAverageSliceDistance;
		features[14] = alignedMaxSliceDistance;
	}
}

void
GeometryFeatureExtractor::computeFeatures(const BranchSegment& branch, std::vector<double>& features) {

	const util::point<double>& sourceCenter  = branch.getSourceSlice()->getComponent()->getCenter();
	const util::point<double>& targetCenter1 = branch.getTargetSlice1()->getComponent()->getCenter();
	const util::point<double>& targetCenter2 = branch.getTargetSlice2()->getComponent()->getCenter();

	unsigned int sourceSize  = branch.getSourceSlice()->getComponent()->getSize();
	unsigned int targetSize1 = branch.getTargetSlice1()->getComponent()->getSize();
	unsigned int targetSize2 = branch.getTargetSlice2()->getComponent()->getSize();
	unsigned int targetSize  = targetSize1 + targetSize2;

	util::point<double> difference = sourceCenter - (targetCenter1*targetSize1 + targetCenter2*targetSize2)/((double)(targetSize));

	double distance = difference.x*difference.x + difference.y*difference.y;


	double overlap = _overlap(*branch.getTargetSlice1(), *branch.getTargetSlice2(), *branch.getSourceSlice());

	double overlapRatio = overlap/(sourceSize + targetSize - overlap);

	double alignedOverlap = _alignedOverlap(*branch.getTargetSlice1(), *branch.getTargetSlice2(), *branch.getSourceSlice());

	double alignedOverlapRatio = alignedOverlap/(sourceSize + targetSize - alignedOverlap);

	double setDifference = (sourceSize - overlap) + (targetSize - overlap);

	double setDifferenceRatio = setDifference/(sourceSize + targetSize);

	double alignedSetDifference = (sourceSize - alignedOverlap) + (targetSize - alignedOverlap);

	double alignedSetDifferenceRatio = alignedSetDifference/(sourceSize + targetSize);

	features[0] = Features::NoFeatureValue;
	features[1] = distance;
	features[2] = setDifference;
	features[3] = setDifferenceRatio;
	features[4] = alignedSetDifference;
	features[5] = alignedSetDifferenceRatio;
	features[6] =
			(branch.getSourceSlice()->getComponent()->getSize() +
			 branch.getTargetSlice1()->getComponent()->getSize() +
			 branch.getTargetSlice2()->getComponent()->getSize())/3.0;
	features[7] = overlap;
	features[8] = overlapRatio;
	features[9] = alignedOverlap;
	features[10] = alignedOverlapRatio;

	if (!_noSliceDistance) {

		double averageSliceDistance, maxSliceDistance;

		_distance(*branch.getTargetSlice1(), *branch.getTargetSlice2(), *branch.getSourceSlice(), true, false, averageSliceDistance, maxSliceDistance);

		double alignedAverageSliceDistance, alignedMaxSliceDistance;

		_distance(*branch.getTargetSlice1(), *branch.getTargetSlice2(), *branch.getSourceSlice(), true, true, alignedAverageSliceDistance, alignedMaxSliceDistance);

		features[11] = averageSliceDistance;
		features[12] = maxSliceDistance;
		features[13] = alignedAverageSliceDistance;
		features[14] = alignedMaxSliceDistance;
	}
}
