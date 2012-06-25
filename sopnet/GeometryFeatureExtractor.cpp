#include <fstream>

#include <boost/filesystem.hpp>

#include <util/helpers.hpp>
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

	bool useCache;

	// try to read features from file
	boost::filesystem::path cachefile("./geometry_features.dat");

	LOG_DEBUG(geometryfeatureextractorlog) << "trying to use cache " << cachefile << std::endl;

	if (boost::filesystem::exists(cachefile)) {

		LOG_DEBUG(geometryfeatureextractorlog) << "reading features from file..." << std::endl;

		std::ifstream in(cachefile.string().c_str());

		boost::archive::binary_iarchive archive(in);

		archive >> *_features;

		useCache = true;

		LOG_DEBUG(geometryfeatureextractorlog) << "done." << std::endl;

	} else {

		LOG_DEBUG(geometryfeatureextractorlog) << "cache file does not exist" << std::endl;

		useCache = false;
	}

	FeatureVisitor featureVisitor;

	unsigned int numChecked = 0;

	foreach (boost::shared_ptr<Segment> segment, *_segments) {

		LOG_ALL(geometryfeatureextractorlog) << "computing features for segment" << segment->getId() << std::endl;

		segment->accept(featureVisitor);

		// check, if cache is still valid
		if (useCache) {

			LOG_ALL(geometryfeatureextractorlog) << "checking consistency of cache" << std::endl;

			if ((*_features).get(segment->getId()) == featureVisitor.getFeatures()) {

				LOG_ALL(geometryfeatureextractorlog) << "found a good entry" << std::endl;

				numChecked++;

			} else {

				LOG_DEBUG(geometryfeatureextractorlog) << "found a bad entry -- will not use cache anymore" << std::endl;

				LOG_ALL(geometryfeatureextractorlog)
						<< "expected " << featureVisitor.getFeatures() << " for segment " << segment->getId()
						<< ", got " << (*_features).get(segment->getId()) << std::endl;

				useCache = false;
			}
		}

		if (!useCache) {

			LOG_ALL(geometryfeatureextractorlog) << "set features to " << featureVisitor.getFeatures() << std::endl;

			_features->add(segment->getId(), featureVisitor.getFeatures());

			LOG_ALL(geometryfeatureextractorlog) << "features are " << (*_features).get(segment->getId()) << std::endl;
		}

		if (numChecked > 10) {

			LOG_DEBUG(geometryfeatureextractorlog) << "found more than 10 good cache samples" << std::endl;
			break;
		}
	}

	// write results to cache file
	if (!useCache) {

		LOG_DEBUG(geometryfeatureextractorlog) << "writing features to file..." << std::endl;

		LOG_ALL(geometryfeatureextractorlog) << "features for segment 14 are " << (*_features).get(14) << std::endl;

		std::ofstream out(cachefile.string().c_str());

		boost::archive::binary_oarchive archive(out);

		archive << *_features;

		LOG_DEBUG(geometryfeatureextractorlog) << "done." << std::endl;
	}

	LOG_ALL(geometryfeatureextractorlog) << "found features: " << *_features << std::endl;

	if (useCache)
		LOG_DEBUG(geometryfeatureextractorlog) << "reusing cache" << std::endl;

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
