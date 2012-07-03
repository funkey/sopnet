#ifndef IMAGEPROCESSING_GRAPH_CUT_PARAMETERS_H__
#define IMAGEPROCESSING_GRAPH_CUT_PARAMETERS_H__

#include <pipeline/all.h>

struct GraphCutParameters : public pipeline::Data {

	GraphCutParameters() :
		pottsWeight(1.0),
		contrastWeight(0.0),
		contrastSigma(0.1),
		eightNeighborhood(true),
		foregroundPrior(0.5){}

	// the weight of the potts-term
	double pottsWeight;

	// the weight of the contrast-term
	double contrastWeight;

	// sensitivity of gray-scale differences to pairwise costs in the contrast
	// term
	double contrastSigma;

	// whether to use a four- or eight-neighborhood
	bool eightNeighborhood;

	// a prior to change the expected number of foreground pixels
	float foregroundPrior;
};

#endif // IMAGEPROCESSING_GRAPH_CUT_PARAMETERS_H__

