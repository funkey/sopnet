#ifndef SOPNET_GEOMETRY_FEATURE_EXTRACTOR_H_
#define SOPNET_GEOMETRY_FEATURE_EXTRACTOR_H_

#include <pipeline/all.h>
#include "Segments.h"
#include "SetDifference.h"
#include "Features.h"

class GeometryFeatureExtractor : public pipeline::SimpleProcessNode {

public:

	GeometryFeatureExtractor();

private:

	template <typename SegmentType>
	void getFeatures(const SegmentType& segment);

	std::vector<double> computeFeatures(const EndSegment& end);

	std::vector<double> computeFeatures(const ContinuationSegment& continuation);

	std::vector<double> computeFeatures(const BranchSegment& branch);

	void updateOutputs();

	pipeline::Input<Segments> _segments;

	pipeline::Output<Features> _features;

	SetDifference _setDifference;

	// number of cache hits
	unsigned int _numChecked;

	bool _useCache;
};

#endif // SOPNET_GEOMETRY_FEATURE_EXTRACTOR_H_

