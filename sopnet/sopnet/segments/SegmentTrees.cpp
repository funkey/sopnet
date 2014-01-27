#include <sopnet/segments/SegmentTrees.h>

unsigned int
SegmentTrees::getNumSections() {

	updateSectionNums();

	return (unsigned int)(_lastSection - _firstSection) + 1;
}

unsigned int
SegmentTrees::getFirstSection() {

	updateSectionNums();

	return (unsigned int)_firstSection;
}

unsigned int
SegmentTrees::getLastSection() {

	updateSectionNums();

	return (unsigned int)_lastSection;
}

void
SegmentTrees::updateSectionNums() {

	if (_lastSection == -1) {

		// for every neuron...
		foreach (boost::shared_ptr<SegmentTree> neuron, _neurons) {

			foreach (boost::shared_ptr<EndSegment> endSegment, neuron->getEnds())
				fit(endSegment->getSlice()->getSection());
			foreach (boost::shared_ptr<ContinuationSegment> continuationSegment, neuron->getContinuations()) {
				fit(continuationSegment->getSourceSlice()->getSection());
				fit(continuationSegment->getTargetSlice()->getSection());
			}
			foreach (boost::shared_ptr<BranchSegment> branchSegment, neuron->getBranches()) {
				fit(branchSegment->getSourceSlice()->getSection());
				fit(branchSegment->getTargetSlice1()->getSection());
				fit(branchSegment->getTargetSlice2()->getSection());
			}
		}
	}
}

void
SegmentTrees::fit(int section) {

	if (_lastSection == -1) {

		_firstSection = section;
		_lastSection  = section;

	} else {

		_firstSection = std::min(_firstSection, section);
		_lastSection  = std::max(_lastSection, section);
	}
}
