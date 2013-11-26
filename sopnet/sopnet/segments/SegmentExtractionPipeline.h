#ifndef SOPNET_SEGMENTS_SEGMENT_EXTRACTION_PIPELINE_H__
#define SOPNET_SEGMENTS_SEGMENT_EXTRACTION_PIPELINE_H__

#include <vector>
#include <string>

#include <imageprocessing/ImageExtractor.h>
#include <imageprocessing/ImageStack.h>
#include <sopnet/inference/LinearConstraints.h>
#include <sopnet/segments/Segments.h>
#include <sopnet/segments/SegmentExtractor.h>
#include <pipeline/Output.h>
#include <pipeline/Value.h>

/**
 * Contains a pipeline with a limb for every inter-section interval, providing 
 * segments and constraints on it. Creates itself from a vector of strings to a 
 * directory or an image stack.
 */
class SegmentExtractionPipeline {

public:

	/**
	 * Create a segment extraction pipeline that uses a directories of slice 
	 * images.
	 *
	 * @param finishLastInterval
	 *             Create special explanation constraints for the last section 
	 *             to avoid border effects.  This is the default in normal 
	 *             operation, but can be disabled for problem dumps.
	 */
	SegmentExtractionPipeline(boost::shared_ptr<std::vector<std::string> > directories, bool finishLastInterval = false);

	/**
	 * Create a segment extraction pipeline that uses an image stack for 
	 * component tree conversion to get the slices.
	 *
	 * @param finishLastInterval
	 *             Create special explanation constraints for the last section 
	 *             to avoid border effects.  This is the default in normal 
	 *             operation, but can be disabled for problem dumps.
	 */
	SegmentExtractionPipeline(boost::shared_ptr<ImageStack> imageStack, pipeline::Value<bool> forceExplanation, bool finishLastInterval = false);

	/**
	 * Get the extracted segments in the given interval.
	 */
	pipeline::OutputBase& getSegments(unsigned int interval);

	/**
	 * Get linear constraints on the segments in the given interval.
	 */
	pipeline::OutputBase& getConstraints(unsigned int interval);

	/**
	 * Get the number of intervals for which the pipeline was created.
	 */
	unsigned int numIntervals() const { return _segmentExtractors.size(); }

private:

	void create();

	// an image stack to image converter for the slice images
	boost::shared_ptr<ImageExtractor> _sliceImageExtractor;

	// a slice extractor for each section
	std::vector<boost::shared_ptr<pipeline::ProcessNode> > _sliceExtractors;

	// a list of slice directories...
	boost::shared_ptr<std::vector<std::string> > _sliceStackDirectories;

	// ...or a stack of images for component tree analysis
	boost::shared_ptr<ImageStack> _slices;

	// the segment extractors for each interval
	std::vector<boost::shared_ptr<SegmentExtractor> > _segmentExtractors;

	pipeline::Value<bool> _forceExplanation;

	// should the last interval be treaded specially?
	bool _finishLastInterval;
};

#endif // SOPNET_SEGMENTS_SEGMENT_EXTRACTION_PIPELINE_H__

