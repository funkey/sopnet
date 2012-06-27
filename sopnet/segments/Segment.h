#ifndef CELLTRACKER_TRACKLET_H__
#define CELLTRACKER_TRACKLET_H__

#include <boost/thread.hpp>

#include <pipeline/all.h>
#include <sopnet/slices/Slice.h>

/**
 * The direction of the segment.
 */
enum Direction {

	Left,
	Right
};

/**
 * A segment represents the connection of slices between sections. Subclasses
 * implement one-to-one segments (continuations), one-to-two segments
 * (branches), and one-to-zero segments (ends).
 */
class Segment : public pipeline::Data {

public:

	Segment(unsigned int id, Direction direction, unsigned int interSectionInterval);

	unsigned int getId() const;

	Direction getDirection() const;

	unsigned int getInterSectionInterval();

	static unsigned int getNextSegmentId();

private:

	static unsigned int NextSegmentId;

	static boost::mutex SegmentIdMutex;

	// a unique id for the segment
	unsigned int _id;

	// the direction of the segment (fixes the meaning of source and target
	// slices in derived classes)
	Direction _direction;

	// the number of the inter-section interval this segment lives in
	unsigned int _interSectionInterval;
};

#endif // CELLTRACKER_TRACKLET_H__

