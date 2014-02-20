#ifndef SOPNET_GROUND_TRUTH_EXTRACTOR_H__
#define SOPNET_GROUND_TRUTH_EXTRACTOR_H__

#include <vector>

#include <pipeline/SimpleProcessNode.h>
#include <imageprocessing/ImageStack.h>
#include <sopnet/slices/Slices.h>
#include <sopnet/segments/Segments.h>
#include <sopnet/features/Overlap.h>

class GroundTruthExtractor : public pipeline::SimpleProcessNode<> {

public:

	/**
	 * Create a ground truth extractor.
	 *
	 * @param firstSection
	 *              The first section to use.
	 * @param lastSection
	 *              The last section to use.
	 * @param addIntensityBoundaries
	 *              Seperate regions of different intensities such that they end 
	 *              up in different slices.
	 */
	GroundTruthExtractor(int firstSection = -1, int lastSection = -1, bool addIntensityBoundaries = true);

private:

	/**
	 * Sorts continuations by source-target overlap descending.
	 */
	struct ContinuationComparator {

		ContinuationComparator() :
			overlap(false, false) {}

		bool operator()(const ContinuationSegment& a, const ContinuationSegment& b) {

			return overlap(*a.getSourceSlice(), *a.getTargetSlice()) > overlap(*b.getSourceSlice(), *b.getTargetSlice());
		}

		Overlap overlap;
	};

	void updateOutputs();

	// extract all slices of each ground-truth section
	std::vector<Slices> extractSlices(int firstSection, int lastSection);

	std::map<float, std::vector<ContinuationSegment> > extractContinuations(const std::vector<Slices>& slices);

	// find a minimal spanning segment tree for each set of slices with the same 
	// id
	Segments findMinimalTrees(const std::vector<Slices>& slices);

	// the ground truth images
	pipeline::Input<ImageStack> _groundTruthSections;

	// continuation and end segments of the ground-truth
	pipeline::Output<Segments> _groundTruthSegments;

	int _firstSection;
	int _lastSection;

	bool _addIntensityBoundaries;
};

#endif // SOPNET_GROUND_TRUTH_EXTRACTOR_H__

