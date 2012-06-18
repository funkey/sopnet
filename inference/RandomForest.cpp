#include <vigra/random_forest_hdf5_impex.hxx>
#include "RandomForest.h"

RandomForest::RandomForest() :
	_outOfBagError(0),
	_variableImportance(0) {

	// emtpy
}

void
RandomForest::prepareTraining(int numSamples, int numFeatures) {

	_samples = SamplesType(SamplesSize(numSamples, numFeatures));
	_labels  = LabelsType(LabelsSize(numSamples, 1));

	_nextSample = 0;

	_variableImportance.resize(numFeatures);

	_numSamples  = numSamples;
	_numFeatures = numFeatures;
}

void
RandomForest::addSample(const std::vector<FeatureType>& sample, LabelType label) {

	for (int i = 0; i < _numFeatures; i++)
		_samples(_nextSample, i) = sample[i];

	_labels(_nextSample) = label;

	_nextSample++;
}

void
RandomForest::train(int numTrees, int numFeatures) {

	if (_nextSample != _numSamples) {

		std::cout << "[RandomForest] incorrect number of samples given: "
		          << _nextSample << " != " << _numSamples << std::endl;
		return;
	}

	// create new random forest instance

	vigra::RandomForestOptions options;

	if (numTrees > 0)
		options.tree_count(numTrees);
	if (numFeatures > 0)
		options.features_per_node(numFeatures);

	_rf = RandomForestType(options);

	// create visitors

	vigra::rf::visitors::VariableImportanceVisitor variableVisitor;
	vigra::rf::visitors::OOB_Error                 errorVisitor;

	_rf.learn(
			_samples,
			_labels,
			vigra::rf::visitors::create_visitor(variableVisitor, errorVisitor));

	_outOfBagError = errorVisitor.oob_breiman;

	for (int i = 0; i < _numFeatures; i++)
		_variableImportance[i] = variableVisitor.variable_importance_(i);

	_numClasses = _rf.class_count();
}

double
RandomForest::getOutOfBagError() {

	return _outOfBagError;
}

std::vector<double>
RandomForest::getVariableImportance() {

	return _variableImportance;
}

int
RandomForest::getLabel(const std::vector<FeatureType>& sample) {

	SamplesType s(SamplesSize(1, _numFeatures));

	for (int i = 0; i < _numFeatures; i++)
		s(i) = sample[i];

	return _rf.predictLabel(s);
}

std::vector<double>
RandomForest::getProbabilities(const std::vector<FeatureType>& sample) {

	SamplesType s(SamplesSize(1, _numFeatures));
	ProbsType   probs(ProbsSize(1, _numClasses));

	for (int i = 0; i < _numFeatures; i++)
		s(i) = sample[i];

	_rf.predictProbabilities(s, probs);

	std::vector<double> p(_numClasses);

	for (int c = 0; c < _numClasses; c++)
		p[c] = probs(c);

	return p;
}

void
RandomForest::write(std::string filename) {

	try {

		vigra::rf_export_HDF5(_rf, filename);

	} catch (std::runtime_error e) {

		std::cerr << "[RandomForest] could not write to file: "
		          << e.what() << std::endl;
	}
}

void
RandomForest::read(std::string filename) {

	try {

		vigra::rf_import_HDF5(_rf, filename);

	} catch (std::runtime_error e) {

		std::cerr << "[RandomForest] could not read from file: "
		          << e.what() << std::endl;
	}

	_numFeatures = _rf.feature_count();
	_numClasses  = _rf.class_count();
}

