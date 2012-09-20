#ifndef SOPNET_OVERLAP_H__
#define SOPNET_OVERLAP_H__

// forward declarations
class Slice;

struct Overlap {

	/**
	 * Compute the overlap between the pixels in slice1 and slice2.
	 *
	 * @param normalized Normalize the result by the sum of the number of pixels
	 *                   in both slices.
	 * @param align Align the slices, such that the centers are on top of each
	 *              other.
	 */
	double operator()(const Slice& slice1, const Slice& slice2, bool normalized = false, bool align = true);

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
	double operator()(const Slice& slice1a, const Slice& slice1b, const Slice& slice2, bool normalized = false, bool align = true);

private:

	unsigned int overlap(
			const ConnectedComponent& c1,
			const ConnectedComponent& c2,
			const util::point<unsigned int>& offset2);
};

#endif // SOPNET_OVERLAP_H__

