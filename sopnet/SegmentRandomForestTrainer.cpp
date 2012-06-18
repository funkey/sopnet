#include "GoldStandardExtractor.h"
#include "SegmentRandomForestTrainer.h"
#include "SegmentFeaturesExtractor.h"

SegmentRandomForestTrainer::SegmentRandomForestTrainer() :
	_goldStandardExtractor(boost::make_shared<GoldStandardExtractor>()),
	_segmentFeaturesExtractor(boost::make_shared<SegmentFeaturesExtractor>()),
	_rfTrainer(boost::make_shared<RandomForestTrainer>()) {

	registerInput(_goldStandardExtractor->getInput("ground truth"), "ground truth");
	registerInput(_allSegments, "all segments");
	registerInput(_segmentFeaturesExtractor->getInput("raw sections"), "raw sections");

	_allSegments.registerBackwardCallback(&SegmentRandomForestTrainer::onSegmentsSet, this);

	_rfTrainer->setInput("positive samples", _goldStandardExtractor->getOutput("gold standard"));
	_rfTrainer->setInput("negative samples", _goldStandardExtractor->getOutput("negative samples"));
	_rfTrainer->setInput("features", _segmentFeaturesExtractor->getOutput("all features"));

	registerOutput(_rfTrainer->getOutput("random forest"), "random forest");
	registerOutput(_goldStandardExtractor->getOutput("gold standard"), "gold standard");
	registerOutput(_goldStandardExtractor->getOutput("negative samples"), "negative samples");
}

void
SegmentRandomForestTrainer::onSegmentsSet(const pipeline::InputSet<Segments>& signal) {

	_goldStandardExtractor->setInput("all segments", _allSegments.getAssignedOutput());
	_segmentFeaturesExtractor->setInput("segments", _allSegments.getAssignedOutput());
}
