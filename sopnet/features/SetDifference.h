#ifndef SOPNET_SET_DIFFERENCE_H__
#define SOPNET_SET_DIFFERENCE_H__

// forward declarations
class Slice;

struct SetDifference {

	/**
	 * Compute the symmetric set difference between the pixels in slice1 and
	 * slice2.
	 *
	 * @param normalized Normalize the result by the sum of the number of pixels
	 *                   in both slices.
	 * @param align Align the slices, such that the centers are on top of each
	 *              other.
	 */
	double operator()(const Slice& slice1, const Slice& slice2, bool normalized = false, bool align = true);

	/**
	 * Compute the symmetric set difference between union of the pixels in
	 * slice1a and slice1b and slice2.
	 *
	 * @param normalized Normalize the result by the sum of the number of pixels
	 *                   in all slices.
	 * @param align Align the slices, such that the centers are on top of each
	 *              other. For slice1a and slice1b, the mean of the centers is
	 *              used.
	 */
	double operator()(const Slice& slice1a, const Slice& slice1b, const Slice& slice2, bool normalized = false, bool align = true);

private:

	unsigned int numDifferent(
			unsigned int size1,
			const std::vector<bool>& pixels,
			const util::point<unsigned int>& size,
			const util::point<double>& centerOffset,
			const util::point<double>& offset,
			const Slice& slice2);
};

#endif // SOPNET_SET_DIFFERENCE_H__

