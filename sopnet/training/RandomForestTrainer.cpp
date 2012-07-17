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
	_gtFeaturesExtractor(boost::make_shared<SegmentFeaturesExtractor>()),
	_rfTrainer(boost::make_shared<SegmentRandomForestTrainer>()) {

	registerInput(_groundTruth, "ground truth");
	registerInput(_allSegments, "all segments");
	registerInput(_rawSections, "raw sections");

	registerOutput(_rfTrainer->getOutput("random forest"), "random forest");
	registerOutput(_goldStandardExtractor->getOutput("gold standard"), "gold standard");
	registerOutput(_goldStandardExtractor->getOutput("negative samples"), "negative samples");

	_groundTruth.registerBackwardCallback(&RandomForestTrainer::onGroundTruthSet, this);
	_allSegments.registerBackwardCallback(&RandomForestTrainer::onSegmentsSet, this);
	_rawSections.registerBackwardCallback(&RandomForestTrainer::onRawSectionsSet, this);
}

void
RandomForestTrainer::onGroundTruthSet(const pipeline::InputSet<Segments>& signal) {

	_goldStandardExtractor->setInput("ground truth", _groundTruth);

	if (optionUseGroundTruth) {

		_rfTrainer->setInput("positive samples", _groundTruth);
		_rfTrainer->setInput("negative samples", _goldStandardExtractor->getOutput("negative samples"));

		// In this case, we have to extract the features for the positive samples.
		_gtFeaturesExtractor->setInput("segments", _groundTruth);
		_rfTrainer->setInput("positive features", _gtFeaturesExtractor->getOutput("all features"));
		_rfTrainer->setInput("negative features", _segmentFeaturesExtractor->getOutput("all features"));

	} else {

		_rfTrainer->setInput("positive samples", _goldStandardExtractor->getOutput("gold standard"));
		_rfTrainer->setInput("negative samples", _goldStandardExtractor->getOutput("negative samples"));

		// we can use the same features for positive and negative segments, here
		_rfTrainer->setInput("positive features", _segmentFeaturesExtractor->getOutput("all features"));
		_rfTrainer->setInput("negative features", _segmentFeaturesExtractor->getOutput("all features"));
	}
}

void
RandomForestTrainer::onSegmentsSet(const pipeline::InputSet<Segments>& signal) {

	_goldStandardExtractor->setInput("all segments", _allSegments);
	_segmentFeaturesExtractor->setInput("segments", _allSegments);
}

void
RandomForestTrainer::onRawSectionsSet(const pipeline::InputSet<ImageStack>& signal) {

	_gtFeaturesExtractor->setInput("raw sections", _rawSections);
	_segmentFeaturesExtractor->setInput("raw sections", _rawSections);
}
