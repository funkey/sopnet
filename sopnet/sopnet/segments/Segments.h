#ifndef CELLTRACKER_TRACKLETS_H__
#define CELLTRACKER_TRACKLETS_H__

#include <external/nanoflann/nanoflann.hpp>

#include <pipeline/all.h>
#include <imageprocessing/ConnectedComponent.h>
#include <imageprocessing/DiscreteVolume.h>
#include "EndSegment.h"
#include "ContinuationSegment.h"
#include "BranchSegment.h"

/**
 * An adaptor class to use std::vector<boost::shared_ptr<SegmentType> > in a
 * nanoflann kd-tree.
 */
template <typename SegmentType>
class SegmentVectorAdaptor {

	typedef std::vector<boost::shared_ptr<SegmentType> > segments_type;

public:

	/**
	 * Create a new adaptor.
	 */
	SegmentVectorAdaptor(const segments_type& segments) :
		_segments(segments) {}

	/**
	 * Nanoflann access interface. Gets the number of data points.
	 */
	size_t kdtree_get_point_count() const { return _segments.size(); }

	/**
	 * Nanoflann access interface. Gets the distance between two data points.
	 */
	inline double kdtree_distance(const double *p1, const size_t index_p2, size_t) const {

		double d0 = p1[0] - _segments[index_p2]->getCenter().x;
		double d1 = p1[1] - _segments[index_p2]->getCenter().y;

		return d0*d0 + d1*d1;
	}

	/**
	 * Nanoflann access interface. Get the 'dim'th component of the 'index'th
	 * data point.
	 */
	inline double kdtree_get_pt(const size_t index, int dim) const {

		if (dim == 0)
			return _segments[index]->getCenter().x;
		else if (dim == 1)
			return _segments[index]->getCenter().y;
		else return 0;
	}

	/**
	 * Nanoflann access interface. Computes a bounding box for the data or
	 * returns false.
	 */
	template <class BBox>
	bool kdtree_get_bbox(BBox&) const { return false; }

private:

	// a reference to the data to sort into the kd-tree
	const segments_type& _segments;
};

class Segments : public pipeline::Data, public DiscreteVolume {

	// nanoflann segment vector adaptors for each segment type
	typedef SegmentVectorAdaptor<EndSegment>          EndSegmentVectorAdaptor;
	typedef SegmentVectorAdaptor<ContinuationSegment> ContinuationSegmentVectorAdaptor;
	typedef SegmentVectorAdaptor<BranchSegment>       BranchSegmentVectorAdaptor;

	// nanoflann kd-tree types for each segment type
	typedef nanoflann::KDTreeSingleIndexAdaptor<
			nanoflann::L2_Simple_Adaptor<double, EndSegmentVectorAdaptor>,
			EndSegmentVectorAdaptor,
			2>
			EndSegmentKdTree;
	typedef nanoflann::KDTreeSingleIndexAdaptor<
			nanoflann::L2_Simple_Adaptor<double, ContinuationSegmentVectorAdaptor>,
			ContinuationSegmentVectorAdaptor,
			2>
			ContinuationSegmentKdTree;
	typedef nanoflann::KDTreeSingleIndexAdaptor<
			nanoflann::L2_Simple_Adaptor<double, BranchSegmentVectorAdaptor>,
			BranchSegmentVectorAdaptor,
			2>
			BranchSegmentKdTree;

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
	 * Add a vector of segments of type SegmentType to this set of segments.
	 */
	template <typename SegmentType>
	void addAll(const std::vector<boost::shared_ptr<SegmentType> >& segments) {

		foreach (boost::shared_ptr<SegmentType> segment, segments)
			add(segment);
	}

	/**
	 * Get all end segments.
	 */
	std::vector<boost::shared_ptr<EndSegment> > getEnds() const;

	/**
	 * Get all continuation segments.
	 */
	std::vector<boost::shared_ptr<ContinuationSegment> > getContinuations() const;

	/**
	 * Get all branch segments.
	 */
	std::vector<boost::shared_ptr<BranchSegment> > getBranches() const;

	/**
	 * Get all segments.
	 */
	std::vector<boost::shared_ptr<Segment> > getSegments() const;

	/**
	 * Get all segments in the given inter-section interval.
	 */
	std::vector<boost::shared_ptr<Segment> > getSegments(unsigned int interval);

	/**
	 * Get all end segments in the given inter-section interval.
	 */
	std::vector<boost::shared_ptr<EndSegment> >& getEnds(unsigned int interval);

	/**
	 * Get all continuation segments in the given inter-section interval.
	 */
	std::vector<boost::shared_ptr<ContinuationSegment> >& getContinuations(unsigned int interval);

	/**
	 * Get all branch segments in the given inter-section interval.
	 */
	std::vector<boost::shared_ptr<BranchSegment> >& getBranches(unsigned int interval);

	/**
	 * Find all end segments in the given inter-section interval that are close
	 * to another end segment.
	 *
	 * @param reference A segment defining the center of the search region.
	 * @param distance The maximally allowed distance of the segments to the
	 *                 given segments.
	 */
	std::vector<std::pair<boost::shared_ptr<EndSegment>, double> > findEnds(
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
	std::vector<std::pair<boost::shared_ptr<ContinuationSegment>, double> > findContinuations(
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
	std::vector<std::pair<boost::shared_ptr<BranchSegment>, double> > findBranches(
			boost::shared_ptr<BranchSegment> reference,
			double                           distance);

	/**
	 * Find all end segments in the given inter-section interval that are close
	 * to the given position.
	 *
	 * @param center The 2D center of the search.
	 * @param interSectionInterval The inter-section interval to search in.
	 * @param distance The maximally allowed distance of the segments to the
	 *                 given segments.
	 */
	std::vector<std::pair<boost::shared_ptr<EndSegment>, double> > findEnds(
			const util::point<double>& center,
			unsigned int               interSectionInterval,
			double                     distance);

	/**
	 * Find all continuation segments in the given inter-section interval that
	 * are close to the given position.
	 *
	 * @param center The 2D center of the search.
	 * @param interSectionInterval The inter-section interval to search in.
	 * @param distance The maximally allowed distance of the segments to the
	 *                 given segments.
	 */
	std::vector<std::pair<boost::shared_ptr<ContinuationSegment>, double> > findContinuations(
			const util::point<double>& center,
			unsigned int               interSectionInterval,
			double                     distance);

	/**
	 * Find all branch segments in the given inter-section interval that are
	 * close to the given position.
	 *
	 * @param center The 2D center of the search.
	 * @param interSectionInterval The inter-section interval to search in.
	 * @param distance The maximally allowed distance of the segments to the
	 *                 given segments.
	 */
	std::vector<std::pair<boost::shared_ptr<BranchSegment>, double> > findBranches(
			const util::point<double>& center,
			unsigned int               interSectionInterval,
			double                     distance);

	/**
	 * Check whether the given segment is part of this set.
	 */
	bool contains(boost::shared_ptr<EndSegment>          end)          { return contains(getEnds(end->getInterSectionInterval()), end); }
	bool contains(boost::shared_ptr<ContinuationSegment> continuation) { return contains(getContinuations(continuation->getInterSectionInterval()), continuation); }
	bool contains(boost::shared_ptr<BranchSegment>       branch)       { return contains(getBranches(branch->getInterSectionInterval()), branch); }

	/**
	 * Remove the given segment from this set.
	 */
	void remove(boost::shared_ptr<EndSegment>          end)          { remove(getEnds(end->getInterSectionInterval()), end); }
	void remove(boost::shared_ptr<ContinuationSegment> continuation) { remove(getContinuations(continuation->getInterSectionInterval()), continuation); }
	void remove(boost::shared_ptr<BranchSegment>       branch)       { remove(getBranches(branch->getInterSectionInterval()), branch); }
	void remove(boost::shared_ptr<Segment> segment) {
		remove(getEnds(         segment->getInterSectionInterval()), segment);
		remove(getContinuations(segment->getInterSectionInterval()), segment);
		remove(getBranches(     segment->getInterSectionInterval()), segment);
	}

	/**
	 * Get the number of inter-section intervals that are covered by these
	 * segments.
	 *
	 * @return The number of inter-section intervals.
	 */
	unsigned int getNumInterSectionIntervals() const;

	/**
	 * Get the number of segments.
	 */
	unsigned int size();

protected:

	/**
	 * Overwritten from Volume.
	 */
	virtual BoundingBox computeBoundingBox() const;

private:

	static std::vector<boost::shared_ptr<EndSegment> >          EmptyEnds;
	static std::vector<boost::shared_ptr<ContinuationSegment> > EmptyContinuations;
	static std::vector<boost::shared_ptr<BranchSegment> >       EmptyBranches;

	// resize to hold segments in the given number of inter-section intervals
	void resize(int numInterSectionInterval);

	template <typename SegmentType>
	std::vector<boost::shared_ptr<SegmentType> > get(
			const std::vector<std::vector<boost::shared_ptr<SegmentType> > >& allSegments) const {

		std::vector<boost::shared_ptr<SegmentType> > segments;

		foreach (const std::vector<boost::shared_ptr<SegmentType> >& interSegments, allSegments)
			std::copy(interSegments.begin(), interSegments.end(), std::back_inserter(segments));

		return segments;
	}

	template <typename SegmentType, typename SegmentAdaptorType, typename SegmentKdTreeType>
	std::vector<std::pair<boost::shared_ptr<SegmentType>, double> > find(
			const util::point<double>& center,
			unsigned int interSectionInterval,
			double distance,
			const std::vector<std::vector<boost::shared_ptr<SegmentType> > >& allSegments,
			std::vector<SegmentAdaptorType*>& adaptors,
			std::vector<SegmentKdTreeType*>& trees,
			std::vector<bool>& dirty) {

		std::vector<std::pair<boost::shared_ptr<SegmentType>, double> > found;

		// nothing if interSectionInterval >= num intervals or no segments in
		// interval
		if (interSectionInterval >= allSegments.size() || allSegments[interSectionInterval].size() == 0)
			return found;

		// delete dirty kd-tree
		if (dirty[interSectionInterval] && trees[interSectionInterval]) {

			delete trees[interSectionInterval];
			delete adaptors[interSectionInterval];

			trees[interSectionInterval] = 0;
			adaptors[interSectionInterval] = 0;
		}

		// create kd-tree, if it does not exist
		if (!trees[interSectionInterval]) {

			// create segment vector adaptor for this inter-section interval
			adaptors[interSectionInterval] = new SegmentAdaptorType(allSegments[interSectionInterval]);

			// create the tree
			trees[interSectionInterval] = new SegmentKdTreeType(2, *adaptors[interSectionInterval], nanoflann::KDTreeSingleIndexAdaptorParams(10));

			// create index
			trees[interSectionInterval]->buildIndex();
		}

		// find close indices
		std::vector<std::pair<size_t, double> > results;

		double query[2];
		query[0] = center.x;
		query[1] = center.y;

		nanoflann::SearchParams params(0 /* ignored parameter */);

		trees[interSectionInterval]->radiusSearch(&query[0], distance, results, params);

		// fill result vector
		size_t index;
		double dist;

		foreach (boost::tie(index, dist), results)
			found.push_back(std::make_pair(allSegments[interSectionInterval][index], dist));

		return found;
	}

	template <typename SegmentType>
	bool contains(std::vector<boost::shared_ptr<SegmentType> >& segments, boost::shared_ptr<SegmentType> segment) {

		foreach (boost::shared_ptr<SegmentType> s, segments)
			if (s->getId() == segment->getId())
				return true;

		return false;
	}

	template <typename SegmentType>
	bool remove(std::vector<boost::shared_ptr<SegmentType> >& segments, boost::shared_ptr<Segment> segment) {

		typename std::vector<boost::shared_ptr<SegmentType> >::iterator i;
		for (i = segments.begin(); i != segments.end(); i++)
			if ((*i)->getId() == segment->getId())
				break;

		if (i != segments.end()) {

			segments.erase(i);
			setBoundingBoxDirty();
			return true;
		}

		return false;
	}

	// one vector of segments for each inter-section interval and segment type
	std::vector<std::vector<boost::shared_ptr<EndSegment> > >          _ends;
	std::vector<std::vector<boost::shared_ptr<ContinuationSegment> > > _continuations;
	std::vector<std::vector<boost::shared_ptr<BranchSegment> > >       _branches;

	// one nanoflann adaptor for each inter-section interval and segment type
	std::vector<EndSegmentVectorAdaptor*>          _endAdaptors;
	std::vector<ContinuationSegmentVectorAdaptor*> _continuationAdaptors;
	std::vector<BranchSegmentVectorAdaptor*>       _branchAdaptors;

	// one kd-tree for each inter-section interval and segment type
	std::vector<EndSegmentKdTree*>          _endTrees;
	std::vector<ContinuationSegmentKdTree*> _continuationTrees;
	std::vector<BranchSegmentKdTree*>       _branchTrees;

	// indicate that kd-trees have to be (re)build
	std::vector<bool> _endTreeDirty;
	std::vector<bool> _continuationTreeDirty;
	std::vector<bool> _branchTreeDirty;
};

#endif // CELLTRACKER_TRACKLETS_H__

