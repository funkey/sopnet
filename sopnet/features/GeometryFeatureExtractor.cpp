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

	// don't use the cache -- there are some inconsistencies
	_useCache = false;
	//// try to read features from file
	//boost::filesystem::path cachefile("./geometry_features.dat");

	//LOG_DEBUG(geometryfeatureextractorlog) << "trying to use cache " << cachefile << std::endl;

	//if (boost::filesystem::exists(cachefile)) {

		//LOG_DEBUG(geometryfeatureextractorlog) << "reading features from file..." << std::endl;

		//std::ifstream in(cachefile.string().c_str());

		//boost::archive::binary_iarchive archive(in);

		//archive >> *_features;

		//_useCache = true;

		//LOG_DEBUG(geometryfeatureextractorlog) << "done." << std::endl;

	//} else {

		//LOG_DEBUG(geometryfeatureextractorlog) << "cache file does not exist" << std::endl;

		//_useCache = false;
	//}

	_numChecked = 0;

	foreach (boost::shared_ptr<EndSegment> segment, _segments->getEnds()) {

		LOG_ALL(geometryfeatureextractorlog) << "computing features for end segment" << segment->getId() << std::endl;

		getFeatures(*segment);

		if (_numChecked > 10) {

			LOG_DEBUG(geometryfeatureextractorlog) << "found more than 10 good cache samples, will use cache" << std::endl;
			break;
		}
	}

	_numChecked = 0;

	foreach (boost::shared_ptr<ContinuationSegment> segment, _segments->getContinuations()) {

		LOG_ALL(geometryfeatureextractorlog) << "computing features for continuation segment" << segment->getId() << std::endl;

		getFeatures(*segment);

		if (_numChecked > 10) {

			LOG_DEBUG(geometryfeatureextractorlog) << "found more than 10 good cache samples, will use cache" << std::endl;
			break;
		}
	}

	_numChecked = 0;

	foreach (boost::shared_ptr<BranchSegment> segment, _segments->getBranches()) {

		LOG_ALL(geometryfeatureextractorlog) << "computing features for branch segment" << segment->getId() << std::endl;

		getFeatures(*segment);

		if (_numChecked > 10) {

			LOG_DEBUG(geometryfeatureextractorlog) << "found more than 10 good cache samples, will use cache" << std::endl;
			break;
		}
	}

	// write results to cache file
	//if (!_useCache) {

		//LOG_DEBUG(geometryfeatureextractorlog) << "writing features to file..." << std::endl;

		//LOG_ALL(geometryfeatureextractorlog) << "features for segment 14 are " << (*_features).get(14) << std::endl;

		//std::ofstream out(cachefile.string().c_str());

		//boost::archive::binary_oarchive archive(out);

		//archive << *_features;

		//LOG_DEBUG(geometryfeatureextractorlog) << "done." << std::endl;
	//}

	LOG_ALL(geometryfeatureextractorlog) << "found features: " << *_features << std::endl;

	if (_useCache)
		LOG_DEBUG(geometryfeatureextractorlog) << "reusing cache" << std::endl;

	LOG_DEBUG(geometryfeatureextractorlog) << "done" << std::endl;
}

template <typename SegmentType>
void
GeometryFeatureExtractor::getFeatures(const SegmentType& segment) {

	LOG_ALL(geometryfeatureextractorlog) << "computing features for segment" << segment.getId() << std::endl;

	std::vector<double> features = computeFeatures(segment);

	// check, if cache is still valid
	if (_useCache) {

		LOG_ALL(geometryfeatureextractorlog) << "checking consistency of cache" << std::endl;

		try {

			if ((*_features).get(segment.getId()) == features) {

				LOG_ALL(geometryfeatureextractorlog) << "found a good entry" << std::endl;

				_numChecked++;

			} else {

				LOG_DEBUG(geometryfeatureextractorlog) << "found a bad entry -- will not use cache anymore" << std::endl;

				LOG_ALL(geometryfeatureextractorlog)
						<< "expected " << features << " for segment " << segment.getId()
						<< ", got " << (*_features).get(segment.getId()) << std::endl;

				_useCache = false;
			}

		} catch (NoSuchSegment& e) {

			LOG_DEBUG(geometryfeatureextractorlog) << "cache is missing an entry -- will not use cache anymore" << std::endl;

			_useCache = false;
		}
	}

	if (!_useCache) {

		LOG_ALL(geometryfeatureextractorlog) << "set features to " << features << std::endl;

		_features->add(segment.getId(), features);

		LOG_ALL(geometryfeatureextractorlog) << "features are " << (*_features).get(segment.getId()) << std::endl;
	}
}

std::vector<double>
GeometryFeatureExtractor::computeFeatures(const EndSegment& end) {

	std::vector<double> features(4);

	features[0] = Features::None;
	features[1] = Features::None;
	features[2] = Features::None;
	features[3] = end.getSlice()->getComponent()->getSize();

	return features;
}

std::vector<double>
GeometryFeatureExtractor::computeFeatures(const ContinuationSegment& continuation) {

	const util::point<double>& sourceCenter = continuation.getSourceSlice()->getComponent()->getCenter();
	const util::point<double>& targetCenter = continuation.getTargetSlice()->getComponent()->getCenter();

	unsigned int sourceSize = continuation.getSourceSlice()->getComponent()->getSize();
	unsigned int targetSize = continuation.getTargetSlice()->getComponent()->getSize();

	util::point<double> difference = sourceCenter - targetCenter;

	double distance = difference.x*difference.x + difference.y*difference.y;

	double setDifference = _setDifference(*continuation.getSourceSlice(), *continuation.getTargetSlice());

	double setDifferenceRatio = setDifference/(sourceSize + targetSize);

	std::vector<double> features(4);

	features[0] = distance;
	features[1] = setDifference;
	features[2] = setDifferenceRatio;
	features[3] =
			(continuation.getSourceSlice()->getComponent()->getSize() +
			 continuation.getTargetSlice()->getComponent()->getSize())*0.5;

	return features;
}

std::vector<double>
GeometryFeatureExtractor::computeFeatures(const BranchSegment& branch) {

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

	std::vector<double> features(4);

	features[0] = distance;
	features[1] = setDifference;
	features[2] = setDifferenceRatio;
	features[3] =
			(branch.getSourceSlice()->getComponent()->getSize() +
			 branch.getTargetSlice1()->getComponent()->getSize() +
			 branch.getTargetSlice2()->getComponent()->getSize())/3.0;

	return features;
}
