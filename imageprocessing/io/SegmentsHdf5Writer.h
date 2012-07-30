#ifndef IMAGEPROCESSING_IO_SEGMENTS_HDF5_WRITER_H__
#define IMAGEPROCESSING_IO_SEGMENTS_HDF5_WRITER_H__

#include <pipeline/all.h>
#include <util/hdf5.h>
#include <boost/function.hpp>
#include <vector>

#include <sopnet/segments/EndSegment.h>
#include <sopnet/segments/ContinuationSegment.h>
#include <sopnet/segments/BranchSegment.h>
#include <sopnet/segments/Segments.h>

class SegmentsHdf5Writer : public pipeline::SimpleProcessNode {

typedef boost::function<
        void
        (const std::vector<boost::shared_ptr<EndSegment> >&          ends,
         const std::vector<boost::shared_ptr<ContinuationSegment> >& continuations,
         const std::vector<boost::shared_ptr<BranchSegment> >&       branches,
         std::vector<double>& costs)>
        costs_function_type;

public:

	SegmentsHdf5Writer(const H5::Group& group);

	void write();

private:

	void writeEnd(boost::shared_ptr<EndSegment> end);
    void writeContinuation(boost::shared_ptr<ContinuationSegment> continuation);
    void writeBranch(boost::shared_ptr<BranchSegment> branch);

	void updateOutputs();

	pipeline::Input<Segments> _segments;
    pipeline::Input<costs_function_type> _costFunction;
    
	H5::Group _group;
};

#endif // IMAGEPROCESSING_IO_SEGMENTS_HDF5_WRITER_H__

