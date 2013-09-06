#ifndef SOPNET_OVERLAP_H__
#define SOPNET_OVERLAP_H__

#include <util/point.hpp>

// forward declarations
class Slice;
class ConnectedComponent;

struct Overlap {

	/**
	 * Create a new overlap feature functor.
	 *
	 * @param normalized Normalize the result by the sum of the number of pixels
	 *                   in both slices.
	 * @param align Align the slices, such that the centers are on top of each
	 *              other.
	 */
	Overlap(bool normalized = false, bool align = true) :
			_normalized(normalized),
			_align(align) {}

	/**
	 * Compute the overlap between the pixels in slice1 and slice2.
	 *
	 */
	double operator()(const Slice& slice1, const Slice& slice2);

	/**
	 * Compute the overlap between the union of the pixels in slice1a and
	 * slice1b and slice2.
	 *
	 * @param normalized Normalize the result by the sum of the number of pixels
	 *                   in all slices.
	 * @param align Align the slices, such that the centers are on top of each
	 *              other. For slice1a and slice1b, the mean of the centers is
	 *              used.
	 */
	double operator()(const Slice& slice1a, const Slice& slice1b, const Slice& slice2);

	/**
	 * Returns true if the overlap between the given slices exceeds the 
	 * threshold value. This method is on average faster then computing the 
	 * exact overlap and comparing it by hand.
	 *
	 * @param slice1 The first slice.
	 * @param slice2 The second slice.
	 * @param value  The overlap threshold value.
	 */
	bool exceeds(const Slice& slice1, const Slice& slice2, double value);

	/**
	 * Returns true if the overlap between the given slices exceeds the 
	 * threshold value. This method is on average faster then computing the 
	 * exact overlap and comparing it by hand.
	 *
	 * @param slice1 The first slice.
	 * @param slice2 The second slice.
	 * @param value  The overlap threshold value.
	 * @param exceededValue If value was exceeded, this value will contain the 
	 * actual overlap value.
	 */
	bool exceeds(const Slice& slice1, const Slice& slice2, double value, double& exceededValue);

	/**
	 * Returns true if the overlap between the given slices exceeds the 
	 * threshold value. This method is on average faster then computing the 
	 * exact overlap and comparing it by hand.
	 *
	 * @param slice1a The first part of the first slice.
	 * @param slice1b The second part of the first slice.
	 * @param slice2  The second slice.
	 * @param value   The overlap threshold value.
	 */
	bool exceeds(const Slice& slice1a, const Slice& slice1b, const Slice& slice2, double value);

	/**
	 * Normalize an absolute overlap value to the range [0, 1].
	 *
	 * @param slice1 The first slice.
	 * @param slice2 The second slice.
	 * @param overlap The absolute overlap between slice1 and slice2 in pixels.
	 * @return The normalized overlap.
	 */
	static double normalize(const Slice& slice1, const Slice& slice2, unsigned int overlap);

	/**
	 * Normalize an absolute overlap value to the range [0, 1].
	 *
	 * @param slice1a The first part of the first slice.
	 * @param slice1b The second part of the first slice.
	 * @param slice2 The second slice.
	 * @param overlap The absolute overlap between the union of slice1a and 
	 * slice2b and slice2 in pixels.
	 * @return The normalized overlap.
	 */
	static double normalize(const Slice& slice1a, const Slice& slice1b, const Slice& slice2, unsigned int overlap);

private:

	unsigned int overlap(
			const ConnectedComponent& c1,
			const ConnectedComponent& c2,
			const util::point<int>& offset2);

	bool _normalized;

	bool _align;
};

#endif // SOPNET_OVERLAP_H__

