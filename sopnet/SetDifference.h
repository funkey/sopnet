#ifndef SOPNET_SET_DIFFERENCE_H__
#define SOPNET_SET_DIFFERENCE_H__

// forward declarations
class Slice;

struct SetDifference {

	double operator()(const Slice& slice1, const Slice& slice2, bool normalized = false);

	double operator()(const Slice& slice1a, const Slice& slice1b, const Slice& slice2, bool normalized = false);
};

#endif // SOPNET_SET_DIFFERENCE_H__

