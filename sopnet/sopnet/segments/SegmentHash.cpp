#include <algorithm>
#include <boost/shared_ptr.hpp>
#include <boost/functional/hash.hpp>
#include <sopnet/segments/Segment.h>
#include "SegmentHash.h"

SegmentHash
hash_value(const Segment& segment) {

	// continuations and branches should have hashes independent of their 
	// direction
	if (segment.getSlices().size() > 1) {

		std::vector<SliceHash> sliceHashes;

		foreach (boost::shared_ptr<Slice> slice, segment.getSlices())
			sliceHashes.push_back(slice->hashValue());

		return hash_value(sliceHashes);

	// end segments should depend on the direction
	} else {

		SegmentHash hash = segment.getSlices()[0]->hashValue();
		boost::hash_combine(hash, segment.getDirection());

		return hash;
	}

}

SegmentHash
hash_value(std::vector<SliceHash> sliceHashes) {

	SliceHash hash = 0;

	std::sort(sliceHashes.begin(), sliceHashes.end());

	foreach(SliceHash sliceHash, sliceHashes)
		boost::hash_combine(hash, sliceHash);

	return hash;
}
