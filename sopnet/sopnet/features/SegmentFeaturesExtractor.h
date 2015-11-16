#ifndef SOPNET_SEGMENT_FEATURES_EXTRACTOR_H__
#define SOPNET_SEGMENT_FEATURES_EXTRACTOR_H__

#include <pipeline/all.h>
#include <imageprocessing/ImageStack.h>
#include <sopnet/segments/Segments.h>
#include "Features.h"

// forward declaration
class GeometryFeatureExtractor;
class HistogramFeatureExtractor;
class EyetrackFeatureExtractor;
class TypeFeatureExtractor;

class SegmentFeaturesExtractor : public pipeline::ProcessNode {

public:

	SegmentFeaturesExtractor();

private:

	class FeaturesAssembler : public pipeline::SimpleProcessNode<> {

	public:

		FeaturesAssembler();

	private:

		void updateOutputs();

		pipeline::Inputs<Features> _features;

		pipeline::Output<Features> _allFeatures;
	};

	void onInputSet(const pipeline::InputSetBase& signal);

	pipeline::Input<Segments> _segments;

	pipeline::Input<ImageStack> _rawSections;

	boost::shared_ptr<GeometryFeatureExtractor>  _geometryFeatureExtractor;

	boost::shared_ptr<HistogramFeatureExtractor> _histogramFeatureExtractor;

	boost::shared_ptr<EyetrackFeatureExtractor> _eyetrackFeatureExtractor;

	boost::shared_ptr<TypeFeatureExtractor> _typeFeatureExtractor;

	boost::shared_ptr<FeaturesAssembler> _featuresAssembler;
};

#endif // SOPNET_SEGMENT_FEATURES_EXTRACTOR_H__

