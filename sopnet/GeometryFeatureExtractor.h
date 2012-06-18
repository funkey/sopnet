#ifndef SOPNET_GEOMETRY_FEATURE_EXTRACTOR_H_
#define SOPNET_GEOMETRY_FEATURE_EXTRACTOR_H_

#include <pipeline/all.h>
#include "Segments.h"
#include "SegmentVisitor.h"
#include "Features.h"

class GeometryFeatureExtractor : public pipeline::SimpleProcessNode {

public:

	GeometryFeatureExtractor();

private:

	class FeatureVisitor : public SegmentVisitor {

	public:

		FeatureVisitor();

		void visit(const EndSegment& end);

		void visit(const ContinuationSegment& continuation);

		void visit(const BranchSegment& branch);

		std::vector<double> getFeatures();

	private:

		unsigned int computeSetDifference(const Slice& slice1, const Slice& slice2);

		unsigned int computeSetDifference(const Slice& slice1a, const Slice& slice1b, const Slice& slice2);

		std::vector<double> _features;
	};

	void updateOutputs();

	pipeline::Input<Segments> _segments;

	pipeline::Output<Features> _features;
};

#endif // SOPNET_GEOMETRY_FEATURE_EXTRACTOR_H_

