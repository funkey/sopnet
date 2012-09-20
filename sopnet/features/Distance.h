#ifndef SOPNET_FEATURES_DISTANCE_H__
#define SOPNET_FEATURES_DISTANCE_H__

// forward declarations
class Slice;

class Distance {

public:

	/**
	 * Create a new Distance feature funtor.
	 *
	 * @param maxDistance The value to assign pixels that are lying outside the
	 *                    distance map of the slice they are compared to.
	 */
	Distance(double maxDistance) :
		_maxDistance(maxDistance) {}

	/**
	 * Computes the average minimal pixel distance between two slices.
	 *
	 * @param symmetric Perform the computation in both directions and average
	 *                  the result.
	 * @param align Align the slices, such that the centers are on top of each
	 *              other.
	 * @param avgSliceDistance[out] The average minimal distance per pixel.
	 * @param maxSliceDistance[out] The maximal minimal distance of each pixel.
	 *
	 * @return The average minimal distance of a pixel in one slice to any pixel
	 *         in the other slice.
	 */
	void operator()(
			const Slice& slice1,
			const Slice& slice2,
			bool symmetric,
			bool align,
			double& avgSliceDistance,
			double& maxSliceDistance);

	/**
	 * Computes the average minimal pixel distance between two slices and
	 * another one.
	 *
	 * @param symmetric Perform the computation in both directions and average
	 *                  the result.
	 * @param align Align the slices, such that the centers are on top of each
	 *              other.
	 * @param avgSliceDistance[out] The average minimal distance per pixel.
	 * @param maxSliceDistance[out] The maximal minimal distance of each pixel.
	 *
	 * @return The average minimal distance of a pixel in one slice to any pixel
	 *         in the other slice.
	 */
	void operator()(
			const Slice& slice1a,
			const Slice& slice1b,
			const Slice& slice2,
			bool symmetric,
			bool align,
			double& avgSliceDistance,
			double& maxSliceDistance);

private:

	void distance(
			const Slice& slice1,
			const Slice& slice2,
			const util::point<unsigned int>& offset2,
			double& avgSliceDistance,
			double& maxSliceDistance);

	void distance(
			const Slice& s1,
			const Slice& s2a,
			const Slice& s2b,
			const util::point<unsigned int>& offset2,
			double& avgSliceDistance,
			double& maxSliceDistance);

	double _maxDistance;
};

#endif // SOPNET_FEATURES_DISTANCE_H__

