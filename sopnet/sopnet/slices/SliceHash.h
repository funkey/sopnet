#ifndef SOPNET_SLICES_SLICE_HASH_H__
#define SOPNET_SLICES_SLICE_HASH_H__

#include <cstddef>

typedef std::size_t SliceHash;

// forward declaration
class Slice;

SliceHash hash_value(const Slice& slice);

#endif // SOPNET_SLICES_SLICE_HASH_H__

