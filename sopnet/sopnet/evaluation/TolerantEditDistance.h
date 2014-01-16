#ifndef SOPNET_EVALUATION_TOLERANT_EDIT_DISTANCE_H__
#define SOPNET_EVALUATION_TOLERANT_EDIT_DISTANCE_H__

#include <imageprocessing/ImageStack.h>
#include <pipeline/SimpleProcessNode.h>
#include <inference/Solution.h>
#include "LocalToleranceFunction.h"
#include "Errors.h"
#include "Cell.h"

class TolerantEditDistance : public pipeline::SimpleProcessNode<> {

public:

	TolerantEditDistance();

	virtual ~TolerantEditDistance();

private:

	typedef LocalToleranceFunction::cell_t cell_t;

	void updateOutputs();

	void clear();

	void extractCells();

	void findBestCellLabels();

	void findErrors();

	void correctReconstruction();

	void assignIndicatorVariable(unsigned int var, unsigned int cellIndex, float gtLabel, float recLabel);

	std::vector<unsigned int>& getIndicatorsByRec(float recLabel);

	std::vector<unsigned int>& getIndicatorsGtToRec(float gtLabel, float recLabel);

	void assignMatchVariable(unsigned int var, float gtLabel, float recLabel);

	unsigned int getMatchVariable(float gtLabel, float recLabel);

	// is there a background label?
	bool _haveBackgroundLabel;

	// the optional background labels of the ground truth and reconstruction
	float _gtBackgroundLabel;
	float _recBackgroundLabel;

	pipeline::Input<ImageStack> _groundTruth;
	pipeline::Input<ImageStack> _reconstruction;

	pipeline::Output<ImageStack> _correctedReconstruction;
	pipeline::Output<ImageStack> _splitLocations;
	pipeline::Output<ImageStack> _mergeLocations;
	pipeline::Output<ImageStack> _fpLocations;
	pipeline::Output<ImageStack> _fnLocations;
	pipeline::Output<Errors>     _errors;

	// the local tolerance function to use
	LocalToleranceFunction* _toleranceFunction;

	// the extends of the ground truth and reconstruction
	unsigned int _width, _height, _depth;

	// reconstruction label indicators by reconstruction label
	std::map<float, std::vector<unsigned int> > _indicatorVarsByRecLabel;

	// reconstruction label indicators by groundtruth label x reconstruction 
	// label
	std::map<float, std::map<float, std::vector<unsigned int> > > _indicatorVarsByGtToRecLabel;

	// (cell index, new label) by indicator variable
	std::map<unsigned int, std::pair<unsigned int, float> > _labelingByVar;

	// map from ground truth label x reconstruction label to match variable
	std::map<float, std::map<float, unsigned int> > _matchVars;

	// the number of indicator variables in the ILP
	unsigned int _numIndicatorVars;

	// the ILP variables for the number of splits and merges
	unsigned int _splits;
	unsigned int _merges;

	// the solution of the ILP
	pipeline::Value<Solution> _solution;
};

#endif // SOPNET_EVALUATION_TOLERANT_EDIT_DISTANCE_H__

