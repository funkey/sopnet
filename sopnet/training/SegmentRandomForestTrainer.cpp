#include <util/ProgramOptions.h>
#include "SegmentRandomForestTrainer.h"

logger::LogChannel segmentrandomforesttrainerlog("segmentrandomforesttrainerlog", "[SegmentRandomForestTrainer] ");

util::ProgramOption optionNumRandomForests(
		util::_module           = "sopnet",
		util::_long_name        = "numRandomForests",
		util::_description_text = "The number of random forests to use for training.");

SegmentRandomForestTrainer::SegmentRandomForestTrainer() :
	_randomForest(boost::make_shared<RandomForest>()) {

	registerInput(_positiveSamples, "positive samples");
	registerInput(_negativeSamples, "negative samples");
	registerInput(_features, "features");
	registerOutput(_randomForest, "random forest");
}

void
SegmentRandomForestTrainer::updateOutputs() {

	LOG_DEBUG(segmentrandomforesttrainerlog) << "updating random forest classifier..." << std::endl;

	if (_features->size() == 0) {

		LOG_DEBUG(segmentrandomforesttrainerlog) << "I have no features -- skipping training" << std::endl;
		return;
	}

	unsigned int numFeatures = (*_features)[0].size();
	unsigned int numSamples  = _positiveSamples->size() + _negativeSamples->size();

	LOG_DEBUG(segmentrandomforesttrainerlog)
			<< "starting training for " << numSamples
			<< " samples with " << numFeatures << " features" << std::endl;

	_randomForest->prepareTraining(numSamples, numFeatures);

	LOG_DEBUG(segmentrandomforesttrainerlog) << "setting samples..." << std::endl;

	foreach (boost::shared_ptr<EndSegment> segment, _positiveSamples->getEnds())
		_randomForest->addSample(_features->get(segment->getId()), 1);

	foreach (boost::shared_ptr<ContinuationSegment> segment, _positiveSamples->getContinuations())
		_randomForest->addSample(_features->get(segment->getId()), 1);

	foreach (boost::shared_ptr<BranchSegment> segment, _positiveSamples->getBranches())
		_randomForest->addSample(_features->get(segment->getId()), 1);

	foreach (boost::shared_ptr<EndSegment> segment, _negativeSamples->getEnds())
		_randomForest->addSample(_features->get(segment->getId()), 0);

	foreach (boost::shared_ptr<ContinuationSegment> segment, _negativeSamples->getContinuations())
		_randomForest->addSample(_features->get(segment->getId()), 0);

	foreach (boost::shared_ptr<BranchSegment> segment, _negativeSamples->getBranches())
		_randomForest->addSample(_features->get(segment->getId()), 0);

	if (optionNumRandomForests) {

		LOG_DEBUG(segmentrandomforesttrainerlog)
				<< "training using " << optionNumRandomForests.as<int>()
				<< " trees..." << std::endl;

		_randomForest->train(optionNumRandomForests);

	} else {

		LOG_DEBUG(segmentrandomforesttrainerlog)
				<< "training (with auto-selection of number of trees)"
				<< std::endl;

		_randomForest->train();
	}

	LOG_DEBUG(segmentrandomforesttrainerlog)
			<< "training finished with OOB: "
			<< _randomForest->getOutOfBagError()
			<< std::endl;
}
