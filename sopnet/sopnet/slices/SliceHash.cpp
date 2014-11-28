#include "Slice.h"
#include "SliceHash.h"
#include <imageprocessing/ConnectedComponent.h>
#include <boost/functional/hash.hpp>

unsigned int SliceHashConfiguration::sectionOffset = 0;

SliceHash hash_value(const Slice& slice) {

	SliceHash hash = slice.getComponent()->hashValue();
	// Correct for the section offset. This is needed if we process a substack 
	// that doesn't start with the first section, but we still want globally 
	// consistant slice hashes.
	boost::hash_combine(hash, boost::hash_value(slice.getSection() + SliceHashConfiguration::sectionOffset));
	return hash;
}
