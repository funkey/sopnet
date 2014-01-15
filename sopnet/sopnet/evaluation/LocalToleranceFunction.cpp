#include "LocalToleranceFunction.h"

void
LocalToleranceFunction::clear() {

	_groundTruthLabels.clear();
	_reconstructionLabels.clear();
	_cells = boost::make_shared<std::vector<cell_t> >();
	_possibleGroundTruthMatches.clear();
	_possibleReconstructionMatches.clear();
	_cellsByRecToGtLabel.clear();
}

std::set<float>&
LocalToleranceFunction::getReconstructionLabels() {

	return _reconstructionLabels;
}

std::set<float>&
LocalToleranceFunction::getGroundTruthLabels() {

	return _groundTruthLabels;
}

void
LocalToleranceFunction::registerPossibleMatch(float gtLabel, float recLabel) {

	_possibleGroundTruthMatches[gtLabel].insert(recLabel);
	_possibleReconstructionMatches[recLabel].insert(gtLabel);
	_groundTruthLabels.insert(gtLabel);
	_reconstructionLabels.insert(recLabel);
}

std::set<float>&
LocalToleranceFunction::getPossibleMatchesByGt(float gtLabel) {

	return _possibleGroundTruthMatches[gtLabel];
}

std::set<float>&
LocalToleranceFunction::getPossibleMathesByRec(float recLabel) {

	return _possibleReconstructionMatches[recLabel];
}
