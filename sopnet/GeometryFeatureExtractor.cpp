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

	double setDifference = computeSetDifference(*continuation.getSourceSlice(), *continuation.getTargetSlice());

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

	double setDifference = computeSetDifference(*branch.getTargetSlice1(), *branch.getTargetSlice2(), *branch.getSourceSlice());

	double setDifferenceRatio = setDifference/(sourceSize + targetSize1 + targetSize2);

	_features[0] = distance;
	_features[1] = setDifference;
	_features[2] = setDifferenceRatio;
	_features[3] = Features::None;
}

unsigned int
GeometryFeatureExtractor::FeatureVisitor::computeSetDifference(const Slice& slice1, const Slice& slice2) {

	const util::rect<double>& bb = slice1.getComponent()->getBoundingBox();

	util::point<unsigned int> offset(static_cast<unsigned int>(bb.minX), static_cast<unsigned int>(bb.minY));
	util::point<unsigned int> size(static_cast<unsigned int>(bb.width() + 2), static_cast<unsigned int>(bb.height() + 2));

	std::vector<bool> pixels(size.x*size.y, false);

	foreach (const util::point<unsigned int>& pixel, slice1.getComponent()->getPixels()) {

		unsigned int x = pixel.x - offset.x;
		unsigned int y = pixel.y - offset.y;

		pixels[x + y*size.x] = true;
	}

	util::point<double> centerOffset = slice1.getComponent()->getCenter() - slice2.getComponent()->getCenter();

	unsigned int different = 0;

	foreach (const util::point<unsigned int>& pixel, slice2.getComponent()->getPixels()) {

		unsigned int x = pixel.x - centerOffset.x - offset.x;
		unsigned int y = pixel.y - centerOffset.x - offset.y;

		if (x < 0 || x >= size.x || y < 0 || y >= size.y || pixels[x + y*size.x] != true)
			different++;
	}

	return different;
}
unsigned int
GeometryFeatureExtractor::FeatureVisitor::computeSetDifference(const Slice& slice1a, const Slice& slice1b, const Slice& slice2) {

	const util::rect<double>& bba = slice1a.getComponent()->getBoundingBox();
	const util::rect<double>& bbb = slice1b.getComponent()->getBoundingBox();

	util::rect<double> bb(std::min(bba.minX, bbb.minY), std::min(bba.minY, bbb.minY), std::max(bba.maxX, bbb.maxX), std::max(bba.maxY, bbb.maxY));

	util::point<unsigned int> offset(static_cast<unsigned int>(bb.minX), static_cast<unsigned int>(bb.minY));
	util::point<unsigned int> size(static_cast<unsigned int>(bb.width() + 2), static_cast<unsigned int>(bb.height() + 2));

	std::vector<bool> pixels(size.x*size.y, false);

	foreach (const util::point<unsigned int>& pixel, slice1a.getComponent()->getPixels()) {

		unsigned int x = pixel.x - offset.x;
		unsigned int y = pixel.y - offset.y;

		pixels[x + y*size.x] = true;
	}
	foreach (const util::point<unsigned int>& pixel, slice1b.getComponent()->getPixels()) {

		unsigned int x = pixel.x - offset.x;
		unsigned int y = pixel.y - offset.y;

		pixels[x + y*size.x] = true;
	}

	util::point<double> centerOffset =
			(slice1a.getComponent()->getCenter()*slice1a.getComponent()->getSize()
			 +
			 slice1b.getComponent()->getCenter()*slice1b.getComponent()->getSize())
			/
			(double)(slice1a.getComponent()->getSize() + slice1b.getComponent()->getSize())
			-
			slice2.getComponent()->getCenter();

	unsigned int different = 0;

	foreach (const util::point<unsigned int>& pixel, slice2.getComponent()->getPixels()) {

		unsigned int x = pixel.x - centerOffset.x - offset.x;
		unsigned int y = pixel.y - centerOffset.x - offset.y;

		if (x < 0 || x >= size.x || y < 0 || y >= size.y || pixels[x + y*size.x] != true)
			different++;
	}

	return different;
}

std::vector<double>
GeometryFeatureExtractor::FeatureVisitor::getFeatures() {

	return _features;
}
