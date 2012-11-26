#include <sopnet/segments/SegmentTrees.h>

unsigned int
SegmentTrees::getNumSections() {

	int start = 0;
	int end   = -1;

	// for every neuron...
	foreach (boost::shared_ptr<SegmentTree> neuron, _neurons) {

		// ...it is enough to consider the end segments
		foreach (boost::shared_ptr<EndSegment> endSegment, neuron->getEnds()) {

			int section = endSegment->getSlice()->getSection();

			if (end == -1) {

				start = section;
				end   = section;

			} else {

				start = std::min(start, section);
				end   = std::max(end, section);
			}
		}
	}

	return (end - start) + 1;
}
