#ifndef SOPNET_GEOMETRY_FEATURE_EXTRACTOR_H_
#define SOPNET_GEOMETRY_FEATURE_EXTRACTOR_H_

#include <pipeline/all.h>
#include <sopnet/segments/Segments.h>
#include "Distance.h"
#include "Features.h"
#include "Overlap.h"
#include "SetDifference.h"

class GeometryFeatureExtractor : public pipeline::SimpleProcessNode<> {

public:

	GeometryFeatureExtractor();

private:

	template <typename SegmentType>
	void getFeatures(const SegmentType& segment);

	void computeFeatures(const EndSegment& end, std::vector<double>& features);

	void computeFeatures(const ContinuationSegment& continuation, std::vector<double>& features);

	void computeFeatures(const BranchSegment& branch, std::vector<double>& features);

	void updateOutputs();

	pipeline::Input<Segments> _segments;

	pipeline::Output<Features> _features;

	SetDifference _setDifference;

	Overlap _overlap;
	Overlap _alignedOverlap;

	Distance _distance;

	bool _noSliceDistance;
};

#endif // SOPNET_GEOMETRY_FEATURE_EXTRACTOR_H_

