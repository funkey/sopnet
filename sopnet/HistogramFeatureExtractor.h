#ifndef SOPNET_HISTOGRAM_FEATURE_EXTRACTOR_H_
#define SOPNET_HISTOGRAM_FEATURE_EXTRACTOR_H_

#include <pipeline/all.h>
#include <imageprocessing/ImageStack.h>
#include "Segments.h"
#include "Features.h"

class HistogramFeatureExtractor : public pipeline::SimpleProcessNode {

public:

	HistogramFeatureExtractor(unsigned int numBins);

private:

	void updateOutputs();

	std::vector<double> getFeatures(const EndSegment& end);

	std::vector<double> getFeatures(const ContinuationSegment& continuation);

	std::vector<double> getFeatures(const BranchSegment& branch);

	std::vector<double> computeHistogram(const Slice& slice);

	pipeline::Input<Segments> _segments;

	pipeline::Input<ImageStack> _sections;

	pipeline::Output<Features> _features;

	unsigned int _numBins;
};

#endif // SOPNET_HISTOGRAM_FEATURE_EXTRACTOR_H_

