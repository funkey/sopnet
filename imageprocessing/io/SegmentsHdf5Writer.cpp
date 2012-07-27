#include <boost/lexical_cast.hpp>

#include "SegmentsHdf5Writer.h"

logger::LogChannel segmentshdf5writerlog("segmentshdf5writerlog", "[SegmentsHdf5Writer] ");

SegmentsHdf5Writer::SegmentsHdf5Writer(const H5::Group& group) :
	_group(group) {

	registerInput(_segments, "segments");
    registerInput(_costFunction, "cost function");
    
}
    
void
SegmentsHdf5Writer::writeEnd(boost::shared_ptr<EndSegment> end ) {
    
}

void
SegmentsHdf5Writer::writeContinuation(boost::shared_ptr<ContinuationSegment> continuation ) {
    
    // continuation->getSourceSlice()->getId()
    
}

void
SegmentsHdf5Writer::writeBranch(boost::shared_ptr<BranchSegment> branch ) {
    
    
}


void
SegmentsHdf5Writer::write() {

    updateInputs();
    
    foreach (boost::shared_ptr<EndSegment> end, _segments->getEnds()) {
        writeEnd(end);
    }
    
    foreach (boost::shared_ptr<ContinuationSegment> continuation, _segments->getContinuations()) {
        writeContinuation(continuation);
    }
    
    foreach (boost::shared_ptr<BranchSegment> branch, _segments->getBranches()) {
        writeContinuation(branch);
    }
    
    std::vector<double> costs;
    (*_costFunction)(_segments->getEnds(), _segments->getContinuations(), _segments->getBranches(), costs);

    hdf5::write(group, "costs", costs, costs.size() );

}

void
SegmentsHdf5Writer::updateOutputs() {

}

