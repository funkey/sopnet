#ifndef CELLTRACKER_TRACKLETS_H__
#define CELLTRACKER_TRACKLETS_H__

#include <pipeline.h>
#include "Segment.h"

class Segments : public pipeline::Data {

	// Segment objects are shared between Segments
	typedef std::vector<boost::shared_ptr<Segment> > segments_type;

public:

	typedef segments_type::iterator         iterator;

	typedef segments_type::const_iterator   const_iterator;

	/**
	 * Remove all segments.
	 */
	void clear();

	/**
	 * Add a single segment to this set of segments.
	 */
	void add(boost::shared_ptr<Segment> segment);

	/**
	 * Add a set of segments to this set of segments.
	 */
	void addAll(boost::shared_ptr<Segments> segments);

	const const_iterator begin() const { return _segments.begin(); }

	iterator begin() { return _segments.begin(); }

	const const_iterator end() const { return _segments.end(); }

	iterator end() { return _segments.end(); }

	unsigned int size() { return _segments.size(); }

	boost::shared_ptr<Segment> operator[](unsigned int i) { return _segments[i]; }

private:

	segments_type _segments;
};

#endif // CELLTRACKER_TRACKLETS_H__

