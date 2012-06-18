#ifndef SOPNET_HISTOGRAM_FEATURE_EXTRACTOR_H_
#define SOPNET_HISTOGRAM_FEATURE_EXTRACTOR_H_

#include <pipeline.h>
#include <imageprocessing/ImageStack.h>
#include "Segments.h"
#include "SegmentVisitor.h"
#include "Features.h"

class HistogramFeatureExtractor : public pipeline::SimpleProcessNode {

public:

	HistogramFeatureExtractor(unsigned int numBins);

private:

	class FeatureVisitor : public SegmentVisitor {

	public:

		FeatureVisitor(unsigned int numBins, ImageStack& sections);

		void visit(const EndSegment& end);

		void visit(const ContinuationSegment& continuation);

		void visit(const BranchSegment& branch);

		std::vector<double> getFeatures();

	private:

		std::vector<double> computeHistogram(const Slice& slice);

		std::vector<double> _features;

		unsigned int _numBins;

		ImageStack& _sections;
	};

	void updateOutputs();

	pipeline::Input<Segments> _segments;

	pipeline::Input<ImageStack> _sections;

	pipeline::Output<Features> _features;

	unsigned int _numBins;
};

#endif // SOPNET_HISTOGRAM_FEATURE_EXTRACTOR_H_

