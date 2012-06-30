#include <imageprocessing/ImageExtractor.h>
#include <sopnet/slices/SliceExtractor.h>
#include "GroundTruthSegmentExtractor.h"
#include "GroundTruthExtractor.h"

logger::LogChannel groundtruthextractorlog("groundtruthextractorlog", "[GroundTruthExtractor] ");

GroundTruthExtractor::GroundTruthExtractor(int firstSection, int lastSection) :
	_sectionExtractor(boost::make_shared<ImageExtractor>()),
	_segmentsAssembler(boost::make_shared<SegmentsAssembler>()),
	_firstSection(firstSection),
	_lastSection(lastSection) {

	registerInput(_groundTruthSections, "ground truth sections");
	registerOutput(_segmentsAssembler->getOutput("ground truth segments"), "ground truth segments");

	_groundTruthSections.registerBackwardCallback(&GroundTruthExtractor::onInputSet, this);
}

void
GroundTruthExtractor::onInputSet(const pipeline::InputSet<ImageStack>& signal) {

	LOG_DEBUG(groundtruthextractorlog) << "ground truth sections set" << std::endl;

	createPipeline();
}

void
GroundTruthExtractor::createPipeline() {

	// clear previous pipeline
	_sliceExtractors.clear();
	_segmentExtractors.clear();
	_segmentsAssembler->clearInputs("segments");

	_sectionExtractor->setInput(_groundTruthSections.getAssignedOutput());

	unsigned int firstSection = (_firstSection >= 0 ? _firstSection : 0);
	unsigned int lastSection  = (_lastSection  >= 0 ? _lastSection  : _groundTruthSections->size() - 1);

	LOG_ALL(groundtruthextractorlog)
			<< "extacting groundtruth from sections "
			<< firstSection << " - " << lastSection
			<< std::endl;

	// create mser parameters suitable to extract ground-truth connected
	// components
	boost::shared_ptr<MserParameters> mserParameters = boost::make_shared<MserParameters>();
	mserParameters->delta        = 1;
	mserParameters->minArea      = 50; // this is to avoid this tiny annotation that mess up the result
	mserParameters->maxArea      = 10000000;
	mserParameters->maxVariation = 100;
	mserParameters->minDiversity = 0;
	mserParameters->darkToBright = false;
	mserParameters->brightToDark = true;

	for (unsigned int section = firstSection; section <= lastSection; section++) {

		LOG_ALL(groundtruthextractorlog) << "creating pipeline for section " << section << std::endl;

		// create a SliceExtractor
		boost::shared_ptr<SliceExtractor> sliceExtractor = boost::make_shared<SliceExtractor>(section);

		// give it the section it has to process and our mser parameters
		sliceExtractor->setInput("membrane", _sectionExtractor->getOutput(section));
		sliceExtractor->setInput("mser parameters", mserParameters);

		// store it in the list of all slice extractors
		_sliceExtractors.push_back(sliceExtractor);

		if (_sliceExtractors.size() <= 1)
			continue;

		// get the previous slice extractor
		boost::shared_ptr<SliceExtractor> prevSliceExtractor = _sliceExtractors[_sliceExtractors.size() - 2];

		// create a segment extractor
		boost::shared_ptr<GroundTruthSegmentExtractor> segmentExtractor = boost::make_shared<GroundTruthSegmentExtractor>();

		// connect current and previous slices to that
		segmentExtractor->setInput("previous slices", prevSliceExtractor->getOutput("slices"));
		segmentExtractor->setInput("next slices", sliceExtractor->getOutput("slices"));

		// store segment extractor
		_segmentExtractors.push_back(segmentExtractor);

		_segmentsAssembler->addInput("segments", segmentExtractor->getOutput("segments"));
	}
}

GroundTruthExtractor::SegmentsAssembler::SegmentsAssembler() {

	registerInputs(_segments, "segments");
	registerOutput(_allSegments, "ground truth segments");
}

void
GroundTruthExtractor::SegmentsAssembler::updateOutputs() {

	LOG_ALL(groundtruthextractorlog)
			<< "assembling segments from "
			<< _segments.size() << " inter-section intervals"
			<< std::endl;

	_allSegments->clear();

	foreach (boost::shared_ptr<Segments> segments, _segments) {

		LOG_ALL(groundtruthextractorlog) << "adding " << segments->size() << " segments" << std::endl;

		_allSegments->addAll(segments);
	}
}

