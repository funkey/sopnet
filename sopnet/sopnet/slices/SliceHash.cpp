#include "Slice.h"
#include "SliceHash.h"
#include <imageprocessing/ConnectedComponent.h>
#include <boost/functional/hash.hpp>

SliceHash hash_value(const Slice& slice) {

	SliceHash hash = slice.getComponent()->hashValue();
	boost::hash_combine(hash, boost::hash_value(slice.getSection()));
	return hash;
}
