#include <imageprocessing/io/ImageStackDirectoryReader.h>
#include <sopnet/slices/SliceExtractor.h>
#include <sopnet/slices/StackSliceExtractor.h>
#include "SegmentExtractionPipeline.h"

static logger::LogChannel segmentextractionpipelinelog("segmentextractionpipelinelog", "[SegmentExtractionPipeline] ");

SegmentExtractionPipeline::SegmentExtractionPipeline(
		boost::shared_ptr<std::vector<std::string> > directories,
		bool finishLastInterval) :
	_sliceStackDirectories(directories),
	_forceExplanation(false),
	_finishLastInterval(finishLastInterval) {

	create();
}

SegmentExtractionPipeline::SegmentExtractionPipeline(
		boost::shared_ptr<ImageStack> imageStack,
		pipeline::Value<bool> forceExplanation,
		bool finishLastInterval) :
	_slices(imageStack),
	_forceExplanation(forceExplanation),
	_finishLastInterval(finishLastInterval) {

	create();
}

pipeline::OutputBase&
SegmentExtractionPipeline::getSegments(unsigned int interval) {

	LOG_ALL(segmentextractionpipelinelog) << "getting segments for interval " << interval << std::endl;

	return _segmentExtractors[interval]->getOutput("segments");
}

pipeline::OutputBase&
SegmentExtractionPipeline::getConstraints(unsigned int interval) {

	LOG_ALL(segmentextractionpipelinelog) << "getting constraints for interval " << interval << std::endl;

	return _segmentExtractors[interval]->getOutput("linear constraints");
}

void
SegmentExtractionPipeline::create() {

	_sliceExtractors.clear();

	unsigned int numSections = 0;

	std::vector<boost::shared_ptr<ImageStackDirectoryReader> > stackSliceReaders;

	if (_sliceStackDirectories) {

		// for every stack directory
		foreach (std::string directory, *_sliceStackDirectories) {

			if (boost::filesystem::is_directory(directory)) {

				numSections++;

				LOG_DEBUG(segmentextractionpipelinelog) << "creating stack reader for " << directory << std::endl;

				// create a new image stack reader
				boost::shared_ptr<ImageStackDirectoryReader> reader = boost::make_shared<ImageStackDirectoryReader>(directory);

				stackSliceReaders.push_back(reader);
			}
		}

	} else {

		numSections = _slices->size();

		_sliceImageExtractor = boost::make_shared<ImageExtractor>();

		// let the internal image extractor know where to look for the image stack
		_sliceImageExtractor->setInput(_slices);
	}

	LOG_DEBUG(segmentextractionpipelinelog) << "creating pipeline for " << numSections << " sections" << std::endl;

	// for every section
	for (unsigned int section = 0; section < numSections; section++) {

		boost::shared_ptr<pipeline::ProcessNode> sliceExtractor;

		LOG_DEBUG(segmentextractionpipelinelog) << "creating pipeline for section " << section << std::endl;

		if (_sliceStackDirectories) {

			// create image stack slice extractor
			sliceExtractor = boost::make_shared<StackSliceExtractor>(section);

			// set its input
			sliceExtractor->setInput("slices", stackSliceReaders[section]->getOutput());

		} else {

			// create a single image slice extractor
			sliceExtractor = boost::make_shared<SliceExtractor<unsigned char> >(section);

			// set its input
			sliceExtractor->setInput("membrane", _sliceImageExtractor->getOutput(section));
		}

		// store it in the list of all slice extractors
		_sliceExtractors.push_back(sliceExtractor);

		if (_sliceExtractors.size() <= 1)
			continue;

		// get the previous slice file reader
		boost::shared_ptr<pipeline::ProcessNode> prevSliceExtractor = _sliceExtractors[_sliceExtractors.size() - 2];

		// create a segment extractor
		boost::shared_ptr<SegmentExtractor> segmentExtractor = boost::make_shared<SegmentExtractor>();

		// connect current and previous slices to that
		segmentExtractor->setInput("previous slices", prevSliceExtractor->getOutput("slices"));
		segmentExtractor->setInput("next slices", sliceExtractor->getOutput("slices"));
		segmentExtractor->setInput("previous conflict sets", prevSliceExtractor->getOutput("conflict sets"));
		segmentExtractor->setInput("force explanation", _forceExplanation);
		if (section == numSections - 1 && _finishLastInterval) // only for the last pair of slices and only if we are not dumping the problem
			segmentExtractor->setInput("next conflict sets", sliceExtractor->getOutput("conflict sets"));

		_segmentExtractors.push_back(segmentExtractor);
	}
}
