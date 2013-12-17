#ifndef SOPNET_EVALUATION_TOLERANT_EDIT_DISTANCE_H__
#define SOPNET_EVALUATION_TOLERANT_EDIT_DISTANCE_H__

#include <imageprocessing/ImageStack.h>
#include <pipeline/SimpleProcessNode.h>
#include "Cell.h"

class TolerantEditDistance : public pipeline::SimpleProcessNode<> {

public:

	TolerantEditDistance();

private:

	typedef Cell<float> cell_t;

	void updateOutputs();

	void extractCells();

	void enumerateCellLabels();

	void findBestCellLabels();

	std::set<float>& getGroundTruthLabels();

	std::set<float>& getReconstructionLabels();

	void clearPossibleMatches();

	void registerPossibleMatch(float gtLabel, float recLabel);

	std::set<float>& getPossibleMatchesByGt(float gtLabel);

	std::set<float>& getPossibleMathesByRec(float recLabel);

	void assignIndicatorVariable(unsigned int var, float gtLabel, float recLabel);

	std::vector<unsigned int>& getIndicatorsByRec(float recLabel);

	std::vector<unsigned int>& getIndicatorsGtToRec(float gtLabel, float recLabel);

	void assignMatchVariable(unsigned int var, float gtLabel, float recLabel);

	unsigned int getMatchVariable(float gtLabel, float recLabel);

	pipeline::Input<ImageStack> _groundTruth;
	pipeline::Input<ImageStack> _reconstruction;

	pipeline::Output<ImageStack> _correctedReconstruction;
	pipeline::Output<ImageStack> _errors;

	// map from rec labels to maps from gt label to cell
	std::map<float, std::map<float, cell_t> > _cells;

	// the extends of the ground truth and reconstruction
	unsigned int _width, _height, _depth;

	float _maxDistanceThreshold;

	// set of all ground truth labels
	std::set<float> _groundTruthLabels;

	// set of all reconstruction labels
	std::set<float> _reconstructionLabels;

	// all possible label matchings, from ground truth to reconstruction
	std::map<float, std::set<float> > _possibleGroundTruthMatches;

	// all possible label matchings, from reconstruction to ground truth
	std::map<float, std::set<float> > _possibleReconstructionMatches;

	// reconstruction label indicators by reconstruction label
	std::map<float, std::vector<unsigned int> > _indicatorVarsByRecLabel;

	// reconstruction label indicators by groundtruth label x reconstruction 
	// label
	std::map<float, std::map<float, std::vector<unsigned int> > > _indicatorVarsByGtToRecLabel;

	// map from ground truth label x reconstruction label to match variable
	std::map<float, std::map<float, unsigned int> > _matchVars;
};

#endif // SOPNET_EVALUATION_TOLERANT_EDIT_DISTANCE_H__

