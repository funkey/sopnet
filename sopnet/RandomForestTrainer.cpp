#include "RandomForestTrainer.h"

logger::LogChannel randomforesttrainerlog("randomforesttrainerlog", "[RandomForestTrainer] ");

RandomForestTrainer::RandomForestTrainer() :
	_randomForest(boost::make_shared<RandomForest>()) {

	registerInput(_positiveSamples, "positive samples");
	registerInput(_negativeSamples, "negative samples");
	registerInput(_features, "features");
	registerOutput(_randomForest, "random forest");
}

void
RandomForestTrainer::updateOutputs() {

	LOG_DEBUG(randomforesttrainerlog) << "updating random forest classifier..." << std::endl;

	if (_features->size() == 0) {

		LOG_DEBUG(randomforesttrainerlog) << "I have no features -- skipping training" << std::endl;
		return;
	}

	unsigned int numFeatures = (*_features)[0].size();
	unsigned int numSamples  = _positiveSamples->size() + _negativeSamples->size();

	LOG_DEBUG(randomforesttrainerlog)
			<< "starting training for " << numSamples
			<< " samples with " << numFeatures << " features" << std::endl;

	_randomForest->prepareTraining(numSamples, numFeatures);

	LOG_DEBUG(randomforesttrainerlog) << "setting samples..." << std::endl;

	foreach (boost::shared_ptr<Segment> segment, *_positiveSamples)
		_randomForest->addSample(_features->get(segment->getId()), 1);

	foreach (boost::shared_ptr<Segment> segment, *_negativeSamples)
		_randomForest->addSample(_features->get(segment->getId()), 0);

	LOG_DEBUG(randomforesttrainerlog) << "training..." << std::endl;

	_randomForest->train();

	LOG_DEBUG(randomforesttrainerlog) << "done" << std::endl;
}
