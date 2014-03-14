#ifndef SOPNET_TRAINING_GOLD_STANDARD_COST_FUNCTION_H__
#define SOPNET_TRAINING_GOLD_STANDARD_COST_FUNCTION_H__

#include <pipeline/SimpleProcessNode.h>
#include <pipeline/Value.h>
#include <sopnet/features/Overlap.h>
#include <sopnet/segments/Segments.h>

// forward declarations
class EndSegment;
class ContinuationSegment;
class BranchSegment;

class GoldStandardCostFunction : public pipeline::SimpleProcessNode<> {

	typedef boost::function<
			void
			(const std::vector<boost::shared_ptr<EndSegment> >&          ends,
			 const std::vector<boost::shared_ptr<ContinuationSegment> >& continuations,
			 const std::vector<boost::shared_ptr<BranchSegment> >&       branches,
			 std::vector<double>& costs)>
			costs_function_type;

public:

	GoldStandardCostFunction();

private:


	void updateOutputs();

	void costs(
			const std::vector<boost::shared_ptr<EndSegment> >&          ends,
			const std::vector<boost::shared_ptr<ContinuationSegment> >& continuations,
			const std::vector<boost::shared_ptr<BranchSegment> >&       branches,
			std::vector<double>& costs);

	double costs(const Segment& segment);

	/**
	 * Get all ground truth segments that overlap the given segment.
	 */
	pipeline::Value<Segments> getOverlappingGroundTruthSegments(const Segment& segment);

	/**
	 * Get the cost for a segment, as if it would not overlap with any ground 
	 * truth segment.
	 */
	double getDefaultCosts(const Segment& segment);

	/**
	 * Get the cost for a segment if it would match the given set of connected 
	 * segments.
	 */
	double getMatchingCosts(const Segment& segment, const Segments& segments);

	/**
	 * Returns true if a and b overlap.
	 */
	bool overlaps(const Segment& a, const Segment& b);

	/**
	 * Append the left and right slices of the given segment to the given lists.
	 */
	void addLeftRightSlices(
			const Segment& segment,
			std::vector<boost::shared_ptr<Slice> >& leftSlices,
			std::vector<boost::shared_ptr<Slice> >& rightSlices);

	/**
	 * Get the sum of sizes of the given slices.
	 */
	int sumSizes(const std::vector<boost::shared_ptr<Slice> >& slices);

	/**
	 * Get the overlap between all slices in a to all slices in b.
	 */
	int overlap(
			const std::vector<boost::shared_ptr<Slice> >& aSlices,
			const std::vector<boost::shared_ptr<Slice> >& bSlices);

	pipeline::Input<Segments> _groundTruth;

	pipeline::Output<costs_function_type> _costFunction;

	Overlap _overlap;
};

#endif // SOPNET_TRAINING_GOLD_STANDARD_COST_FUNCTION_H__

