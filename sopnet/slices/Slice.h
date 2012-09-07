#ifndef CELLTRACKER_CELL_H__
#define CELLTRACKER_CELL_H__

#include <boost/shared_ptr.hpp>

// forward declaration
class ConnectedComponent;

class Slice {

public:

	/**
	 * Create a new slice.
	 *
	 * @param id The id of the new slice.
	 * @param section The section this slice lives in.
	 * @param component The blob that defines the shape of this slice.
	 */
	Slice(
			unsigned int id,
			unsigned int section,
			boost::shared_ptr<ConnectedComponent> component);

	/**
	 * Get the id of this slice.
	 */
	unsigned int getId() const;

	/**
	 * Get the section number, this slice lives in.
	 */
	unsigned int getSection() const;

	/**
	 * Get the blob of this slice.
	 */
	boost::shared_ptr<ConnectedComponent> getComponent() const;

	/**
	 * Intersect this slice with another one. Note that the result might not be
	 * a single connected component any longer.
	 */
	void intersect(const Slice& other);

private:

	unsigned int _id;

	unsigned int _section;

	boost::shared_ptr<ConnectedComponent> _component;
};

#endif // CELLTRACKER_CELL_H__

