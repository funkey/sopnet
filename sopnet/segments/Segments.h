#ifndef CELLTRACKER_TRACKLETS_H__
#define CELLTRACKER_TRACKLETS_H__

#include <external/kdtree++/kdtree.hpp>

#include <pipeline/all.h>
#include <imageprocessing/ConnectedComponent.h>
#include "EndSegment.h"
#include "ContinuationSegment.h"
#include "BranchSegment.h"

class Segments : public pipeline::Data {

	// end segment kd tree
	typedef KDTree::KDTree<
			2,
			boost::shared_ptr<EndSegment>,
			boost::function<double(boost::shared_ptr<EndSegment>, size_t)> >
		end_tree_type;

	// continuation segment kd tree
	typedef KDTree::KDTree<
			2,
			boost::shared_ptr<ContinuationSegment>,
			boost::function<double(boost::shared_ptr<ContinuationSegment>, size_t)> >
		continuation_tree_type;

	// branch segment kd tree
	typedef KDTree::KDTree<
			2,
			boost::shared_ptr<BranchSegment>,
			boost::function<double(boost::shared_ptr<BranchSegment>, size_t)> >
		branch_tree_type;

public:

	~Segments();

	/**
	 * Remove all segments.
	 */
	void clear();

	/**
	 * Add a single end segment to this set of segments.
	 */
	void add(boost::shared_ptr<EndSegment> end);

	/**
	 * Add a single continuation segment to this set of segments.
	 */
	void add(boost::shared_ptr<ContinuationSegment> continuation);

	/**
	 * Add a single branch segment to this set of segments.
	 */
	void add(boost::shared_ptr<BranchSegment> branch);

	/**
	 * Add a set of segments to this set of segments.
	 */
	void addAll(boost::shared_ptr<Segments> segments);

	/**
	 * Get all end segments in the given inter-section interval.
	 */
	std::vector<boost::shared_ptr<EndSegment> > getEnds(int interval = -1);

	/**
	 * Get all continuation segments in the given inter-section interval.
	 */
	std::vector<boost::shared_ptr<ContinuationSegment> > getContinuations(int interval = -1);

	/**
	 * Get all branch segments in the given inter-section interval.
	 */
	std::vector<boost::shared_ptr<BranchSegment> > getBranches(int interval = -1);

	/**
	 * Find all end segments in the given inter-section interval that are close
	 * to another end segment.
	 *
	 * @param reference A segment defining the center of the search region.
	 * @param distance The maximally allowed distance of the segments to the
	 *                 given segments.
	 */
	std::vector<boost::shared_ptr<EndSegment> > findEnds(
			boost::shared_ptr<EndSegment> reference,
			double                        distance);

	/**
	 * Find all continuation segments in the given inter-section interval that
	 * are close to another continuation segment.
	 *
	 * @param reference A segment defining the center of the search region.
	 * @param distance The maximally allowed distance of the segments to the
	 *                 given segments.
	 */
	std::vector<boost::shared_ptr<ContinuationSegment> > findContinuations(
			boost::shared_ptr<ContinuationSegment> reference,
			double                                 distance);

	/**
	 * Find all branch segments in the given inter-section interval that are
	 * close to another branch segment.
	 *
	 * @param reference A segment defining the center of the search region.
	 * @param distance The maximally allowed distance of the segments to the
	 *                 given segments.
	 */
	std::vector<boost::shared_ptr<BranchSegment> > findBranches(
			boost::shared_ptr<BranchSegment> reference,
			double                           distance);

	/**
	 * Get the number of inter-section intervals that are covered by these
	 * segments.
	 *
	 * @return The number of inter-section intervals.
	 */
	unsigned int getNumInterSectionIntervals();

	/**
	 * Get the number of segments.
	 */
	unsigned int size();

private:

	struct EndSegmentCoordinates {

		double operator()(boost::shared_ptr<EndSegment> end, size_t i) {

			const util::point<double>& center = end->getSlice()->getComponent()->getCenter();

			if (i == 0)
				return center.x;

			return center.y;
		}
	};

	struct ContinuationSegmentCoordinates {

		double operator()(boost::shared_ptr<ContinuationSegment> continuation, size_t i) {

			util::point<double> meanCenter =
					(continuation->getSourceSlice()->getComponent()->getCenter() +
					 continuation->getTargetSlice()->getComponent()->getCenter())*0.5;

			if (i == 0)
				return meanCenter.x;

			return meanCenter.y;
		}
	};

	struct BranchSegmentCoordinates {

		double operator()(boost::shared_ptr<BranchSegment> branch, size_t i) {

			util::point<double> meanCenter =
					(branch->getSourceSlice()->getComponent()->getCenter() +
					 (branch->getTargetSlice1()->getComponent()->getCenter() +
					  branch->getTargetSlice2()->getComponent()->getCenter())*0.5)*0.5;

			if (i == 0)
				return meanCenter.x;

			return meanCenter.y;
		}
	};

	template <typename SegmentType>
	void addAll(const std::vector<boost::shared_ptr<SegmentType> >& segments);

	// one end-segment kd-tree per inter-section interval
	std::vector<end_tree_type*> _endTrees;

	// one continue-segment kd-tree per inter-section interval
	std::vector<continuation_tree_type*> _continuationTrees;

	// one branch-segment kd-tree per inter-section interval
	std::vector<branch_tree_type*> _branchTrees;

	// functors to access the coordinates of segments
	EndSegmentCoordinates          _endCoordinates;
	ContinuationSegmentCoordinates _continuationCoordinates;
	BranchSegmentCoordinates       _branchCoordinates;
};

#endif // CELLTRACKER_TRACKLETS_H__

