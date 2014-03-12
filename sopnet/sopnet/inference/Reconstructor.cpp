#include <util/Logger.h>
#include <util/foreach.h>
#include "Reconstructor.h"

static logger::LogChannel reconstructorlog("reconstructorlog", "[Reconstructor] ");

Reconstructor::Reconstructor() :
	_reconstruction(new Segments()) {

	registerInput(_solution, "solution");
	registerInput(_segments, "segments");
	registerOutput(_reconstruction, "reconstruction");
}

void
Reconstructor::updateOutputs() {

	LOG_DEBUG(reconstructorlog) << "reconstructing segments from solution" << std::endl;

	updateReconstruction();
}

void
Reconstructor::updateReconstruction() {

	// remove all previous segment in the reconstruction
	_reconstruction->clear();

	LOG_ALL(reconstructorlog) << "Solution consists of segments: ";

	_currentSegmentNum = 0;

	foreach (boost::shared_ptr<EndSegment> segment, _segments->getEnds())
		probe(segment);

	foreach (boost::shared_ptr<ContinuationSegment> segment, _segments->getContinuations())
		probe(segment);

	foreach (boost::shared_ptr<BranchSegment> segment, _segments->getBranches())
		probe(segment);

	LOG_ALL(reconstructorlog) << std::endl;
}

template <typename SegmentType>
void
Reconstructor::probe(boost::shared_ptr<SegmentType> segment) {

	if (_currentSegmentNum < _solution->size() && (*_solution)[_currentSegmentNum] == 1.0) {

		_reconstruction->add(segment);

		LOG_ALL(reconstructorlog) << segment->getId() << " ";
	}

	_currentSegmentNum++;
}
