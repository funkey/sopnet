#include "ImageExtractor.h"
#include "SliceExtractor.h"
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

	for (unsigned int section = firstSection; section <= lastSection; section++) {

		// create an SliceExtractor
		boost::shared_ptr<SliceExtractor> sliceExtractor = boost::make_shared<SliceExtractor>(section);

		// give it the section it has to process
		sliceExtractor->setInput(_sectionExtractor->getOutput(section));

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

	_allSegments->clear();

	foreach (boost::shared_ptr<Segments> segments, _segments)
		_allSegments->addAll(segments);
}

