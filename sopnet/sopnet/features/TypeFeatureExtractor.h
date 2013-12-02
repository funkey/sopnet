#ifndef SOPNET_TYPE_FEATURE_EXTRACTOR_H_
#define SOPNET_TYPE_FEATURE_EXTRACTOR_H_

#include <pipeline/all.h>
#include <sopnet/segments/Segments.h>
#include "Features.h"

class TypeFeatureExtractor : public pipeline::SimpleProcessNode<> {

public:

	TypeFeatureExtractor();

private:

	template <typename SegmentType>
	void getFeatures(const SegmentType& segment);

	void computeFeatures(const EndSegment& end, std::vector<double>& features);

	void computeFeatures(const ContinuationSegment& continuation, std::vector<double>& features);

	void computeFeatures(const BranchSegment& branch, std::vector<double>& features);

	void updateOutputs();

	pipeline::Input<Segments> _segments;

	pipeline::Output<Features> _features;
};

#endif // SOPNET_TYPE_FEATURE_EXTRACTOR_H_

