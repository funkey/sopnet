#include <util/Logger.h>
#include <util/foreach.h>
#include "Reconstructor.h"

static logger::LogChannel reconstructorlog("reconstructorlog", "[Reconstructor] ");

Reconstructor::Reconstructor() :
	_reconstruction(boost::make_shared<Segments>()) {

	registerInput(_solution, "solution");
	registerInput(_segmentIdsMap, "segment ids map");
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

	unsigned int i = 0;
	foreach (boost::shared_ptr<Segment> segment, *_segments) {

		if ((*_solution)[(*_segmentIdsMap)[segment->getId()]] == 1.0) {

			_reconstruction->add(segment);

			LOG_ALL(reconstructorlog) << segment->getId() << " ";
		}

		i++;
	}

	LOG_ALL(reconstructorlog) << std::endl;
}
