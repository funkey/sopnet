#include <sopnet/features/SegmentFeaturesExtractor.h>
#include <util/ProgramOptions.h>
#include "GoldStandardExtractor.h"
#include "RandomForestTrainer.h"

util::ProgramOption optionUseGroundTruth(
		util::_module           = "sopnet",
		util::_long_name        = "useGroundTruth",
		util::_description_text = "Use the ground-truth segments as positive training samples instead of "
		                          "the gold-standard segments. (The \"gold-standard\" segments are the "
		                          "closest extracted hypotheses to the \"ground-truth\", which is provided "
		                          "as an id map.)");

RandomForestTrainer::RandomForestTrainer() :
	_goldStandardExtractor(boost::make_shared<GoldStandardExtractor>()),
	_segmentFeaturesExtractor(boost::make_shared<SegmentFeaturesExtractor>()),
	_rfTrainer(boost::make_shared<SegmentRandomForestTrainer>()) {

	registerInput(_groundTruth, "ground truth");
	registerInput(_allSegments, "all segments");
	registerInput(_segmentFeaturesExtractor->getInput("raw sections"), "raw sections");

	_groundTruth.registerBackwardCallback(&RandomForestTrainer::onGroundTruthSet, this);
	_allSegments.registerBackwardCallback(&RandomForestTrainer::onSegmentsSet, this);

	_rfTrainer->setInput("negative samples", _goldStandardExtractor->getOutput("negative samples"));
	_rfTrainer->setInput("features", _segmentFeaturesExtractor->getOutput("all features"));

	registerOutput(_rfTrainer->getOutput("random forest"), "random forest");
	registerOutput(_goldStandardExtractor->getOutput("gold standard"), "gold standard");
	registerOutput(_goldStandardExtractor->getOutput("negative samples"), "negative samples");
}

void
RandomForestTrainer::onGroundTruthSet(const pipeline::InputSet<Segments>& signal) {

	_goldStandardExtractor->setInput("ground truth", _groundTruth);

	if (optionUseGroundTruth) {

		_rfTrainer->setInput("positive samples", _groundTruth);

	} else {

		_rfTrainer->setInput("positive samples", _goldStandardExtractor->getOutput("gold standard"));
	}
}

void
RandomForestTrainer::onSegmentsSet(const pipeline::InputSet<Segments>& signal) {

	_goldStandardExtractor->setInput("all segments", _allSegments.getAssignedOutput());
	_segmentFeaturesExtractor->setInput("segments", _allSegments.getAssignedOutput());
}
