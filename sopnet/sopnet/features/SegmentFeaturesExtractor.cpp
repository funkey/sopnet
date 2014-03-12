#include "SegmentFeaturesExtractor.h"
#include "GeometryFeatureExtractor.h"
#include "HistogramFeatureExtractor.h"
#include "TypeFeatureExtractor.h"

logger::LogChannel segmentfeaturesextractorlog("segmentfeaturesextractorlog", "[SegmentFeaturesExtractor] ");

SegmentFeaturesExtractor::SegmentFeaturesExtractor() :
	_geometryFeatureExtractor(boost::make_shared<GeometryFeatureExtractor>()),
	_histogramFeatureExtractor(boost::make_shared<HistogramFeatureExtractor>(10)),
	_typeFeatureExtractor(boost::make_shared<TypeFeatureExtractor>()),
	_featuresAssembler(boost::make_shared<FeaturesAssembler>()) {

	registerInput(_segments, "segments");
	registerInput(_rawSections, "raw sections");

	registerOutput(_featuresAssembler->getOutput("all features"), "all features");

	_segments.registerCallback(&SegmentFeaturesExtractor::onInputSet, this);
	_rawSections.registerCallback(&SegmentFeaturesExtractor::onInputSet, this);

	_featuresAssembler->addInput(_geometryFeatureExtractor->getOutput());
	_featuresAssembler->addInput(_histogramFeatureExtractor->getOutput());
	_featuresAssembler->addInput(_typeFeatureExtractor->getOutput());
}

void
SegmentFeaturesExtractor::onInputSet(const pipeline::InputSetBase&) {

	if (_segments.isSet() && _rawSections.isSet()) {

		_geometryFeatureExtractor->setInput("segments", _segments.getAssignedOutput());
		_histogramFeatureExtractor->setInput("segments", _segments.getAssignedOutput());
		_histogramFeatureExtractor->setInput("raw sections", _rawSections.getAssignedOutput());
		_typeFeatureExtractor->setInput("segments", _segments.getAssignedOutput());
	}
}

SegmentFeaturesExtractor::FeaturesAssembler::FeaturesAssembler() :
	_allFeatures(new Features()) {

	registerInputs(_features, "features");
	registerOutput(_allFeatures, "all features");
}

void
SegmentFeaturesExtractor::FeaturesAssembler::updateOutputs() {

	LOG_DEBUG(segmentfeaturesextractorlog) << "assembling features from " << _features.size() << " feature groups" << std::endl;

	_allFeatures->clear();

	foreach (boost::shared_ptr<Features> features, _features) {

		LOG_ALL(segmentfeaturesextractorlog) << "processing feature group" << std::endl << std::endl << *features << std::endl;

		foreach (const std::string& name, features->getNames()) {

			LOG_ALL(segmentfeaturesextractorlog) << "adding name " << name << std::endl;
			_allFeatures->addName(name);
		}

		if (_allFeatures->size() == 0) {

			LOG_ALL(segmentfeaturesextractorlog) << "initialising all features with " << features->size() << " feature vectors from current feature group" << std::endl;

			unsigned int numFeatures = (features->size() > 0  ? (*features)[0].size() : 0);

			_allFeatures->resize(features->size(), numFeatures);

			unsigned int i = 0;
			foreach (const std::vector<double>& feature, *features) {

				std::copy(feature.begin(), feature.end(), (*_allFeatures)[i].begin());
				i++;
			}

		} else {

			LOG_ALL(segmentfeaturesextractorlog) << "appending " << features->size() << " features from current feature group" << std::endl;

			unsigned int i = 0;
			foreach (const std::vector<double>& feature, *features) {

				unsigned int prevSize = (*_allFeatures)[i].size();
				unsigned int newSize  = prevSize + feature.size();

				(*_allFeatures)[i].resize(newSize);

				std::copy(feature.begin(), feature.end(), (*_allFeatures)[i].begin() + prevSize);

				i++;
			}
		}

		LOG_ALL(segmentfeaturesextractorlog) << "all features are now:" << std::endl << std::endl << *_allFeatures << std::endl;
	}

	// the new features should have the same segment ids map like every features
	_allFeatures->setSegmentIdsMap(_features[0]->getSegmentsIdsMap());
}
