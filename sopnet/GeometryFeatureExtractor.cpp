#include <imageprocessing/ConnectedComponent.h>
#include "GeometryFeatureExtractor.h"
#include "EndSegment.h"
#include "ContinuationSegment.h"
#include "BranchSegment.h"

logger::LogChannel geometryfeatureextractorlog("geometryfeatureextractorlog", "[GeometryFeatureExtractor] ");

GeometryFeatureExtractor::GeometryFeatureExtractor() :
	_features(boost::make_shared<Features>()) {

	registerInput(_segments, "segments");
	registerOutput(_features, "features");

	_features->addName("distance");
	_features->addName("set difference");
	_features->addName("set difference ratio");
	_features->addName("size");
}

void
GeometryFeatureExtractor::updateOutputs() {

	LOG_DEBUG(geometryfeatureextractorlog) << "extracting features" << std::endl;

	_features->clear();

	FeatureVisitor featureVisitor;

	foreach (boost::shared_ptr<Segment> segment, *_segments) {

		segment->accept(featureVisitor);
		_features->add(segment->getId(), featureVisitor.getFeatures());
	}

	LOG_ALL(geometryfeatureextractorlog) << "found features: " << *_features << std::endl;

	LOG_DEBUG(geometryfeatureextractorlog) << "done" << std::endl;
}

GeometryFeatureExtractor::FeatureVisitor::FeatureVisitor() :
	_features(4) {}

void
GeometryFeatureExtractor::FeatureVisitor::visit(const EndSegment& end) {

	_features[0] = Features::None;
	_features[1] = Features::None;
	_features[2] = Features::None;
	_features[3] = end.getSlice()->getComponent()->getSize();
}

void
GeometryFeatureExtractor::FeatureVisitor::visit(const ContinuationSegment& continuation) {

	const util::point<double>& sourceCenter = continuation.getSourceSlice()->getComponent()->getCenter();
	const util::point<double>& targetCenter = continuation.getTargetSlice()->getComponent()->getCenter();

	unsigned int sourceSize = continuation.getSourceSlice()->getComponent()->getSize();
	unsigned int targetSize = continuation.getTargetSlice()->getComponent()->getSize();

	util::point<double> difference = sourceCenter - targetCenter;

	double distance = difference.x*difference.x + difference.y*difference.y;

	double setDifference = _setDifference(*continuation.getSourceSlice(), *continuation.getTargetSlice());

	double setDifferenceRatio = setDifference/(sourceSize + targetSize);

	_features[0] = distance;
	_features[1] = setDifference;
	_features[2] = setDifferenceRatio;
	_features[3] = Features::None;
}

void
GeometryFeatureExtractor::FeatureVisitor::visit(const BranchSegment& branch) {

	const util::point<double>& sourceCenter  = branch.getSourceSlice()->getComponent()->getCenter();
	const util::point<double>& targetCenter1 = branch.getTargetSlice1()->getComponent()->getCenter();
	const util::point<double>& targetCenter2 = branch.getTargetSlice2()->getComponent()->getCenter();

	unsigned int sourceSize  = branch.getSourceSlice()->getComponent()->getSize();
	unsigned int targetSize1 = branch.getTargetSlice1()->getComponent()->getSize();
	unsigned int targetSize2 = branch.getTargetSlice2()->getComponent()->getSize();

	util::point<double> difference = sourceCenter - (targetCenter1*targetSize1 + targetCenter2*targetSize2)/((double)(targetSize1 + targetSize2));

	double distance = difference.x*difference.x + difference.y*difference.y;

	double setDifference = _setDifference(*branch.getTargetSlice1(), *branch.getTargetSlice2(), *branch.getSourceSlice());

	double setDifferenceRatio = setDifference/(sourceSize + targetSize1 + targetSize2);

	_features[0] = distance;
	_features[1] = setDifference;
	_features[2] = setDifferenceRatio;
	_features[3] = Features::None;
}

std::vector<double>
GeometryFeatureExtractor::FeatureVisitor::getFeatures() {

	return _features;
}
