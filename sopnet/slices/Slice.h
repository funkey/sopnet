#ifndef CELLTRACKER_CELL_H__
#define CELLTRACKER_CELL_H__

#include <boost/shared_ptr.hpp>

#include <vigra/multi_array.hxx>

#include <util/ProgramOptions.h>
#include <util/rect.hpp>

// forward declaration
class ConnectedComponent;

class Slice {

	typedef vigra::MultiArray<2, float> distance_map_type;

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
	 * Get the distance map of this slice. Up to a certain distance, this shows
	 * the minimal distance of a pixel to any pixel of the slice.
	 */
	const distance_map_type& getDistanceMap() const { return _distanceMap; }

	/**
	 * Get the bounding box of the distance map.
	 */
	const util::rect<unsigned int>& getDistanceMapBoundingBox() const { return _distanceMapSize; }

	/**
	 * Intersect this slice with another one. Note that the result might not be
	 * a single connected component any longer.
	 */
	void intersect(const Slice& other);

	/**
	 * Provide access to this program option to other modules.
	 */
	static util::ProgramOption optionMaxDistanceMapValue;

private:

	void computeDistanceMap();

	unsigned int _id;

	unsigned int _section;

	boost::shared_ptr<ConnectedComponent> _component;

	// a distance map for pixels surrounding this slice
	distance_map_type _distanceMap;

	// the upper left corner and extends of the distance map in the section
	util::rect<unsigned int> _distanceMapSize;
};

#endif // CELLTRACKER_CELL_H__

