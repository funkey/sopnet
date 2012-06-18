#ifndef CELLTRACKER_TRACKLET_H__
#define CELLTRACKER_TRACKLET_H__

#include <boost/thread.hpp>

#include <pipeline/all.h>
#include "Slice.h"

// forward declaration
class SegmentVisitor;

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

	Segment(unsigned int id, Direction direction);

	unsigned int getId() const;

	virtual void accept(SegmentVisitor& visitor) = 0;

	virtual void accept(SegmentVisitor& visitor) const = 0;

	Direction getDirection() const;

	static unsigned int getNextSegmentId();

private:

	static unsigned int NextSegmentId;

	static boost::mutex SegmentIdMutex;

	unsigned int _id;

	Direction _direction;
};

#endif // CELLTRACKER_TRACKLET_H__

