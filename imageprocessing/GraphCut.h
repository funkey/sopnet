#ifndef IMAGEPROCESSING_GRAPH_CUT_H__
#define IMAGEPROCESSING_GRAPH_CUT_H__

#include <external/dgc/graph.h>

#include <imageprocessing/Image.h>
#include <pipeline/all.h>
#include <util/Logger.h>

#include "GraphCutParameters.h"


class GraphCut : public pipeline::SimpleProcessNode<> {

	typedef Graph<float,float,float> graph_type;

public:

	GraphCut();

private:

	void updateOutputs();

	void onModifiedImage(const pipeline::Modified& signal);

	void onModifiedGCParameters(const pipeline::Modified& signal);

	void onModifiedPottsImage(const pipeline::Modified& signal);

	void doMaxFlow();

	void setTerminalWeights();

	void setEdgeWeights();

	void getSegmentation();

	float getCapacity(float probability, float foreground);

	int getNodeId(int x, int y);

	double getPairwiseCosts(int x1, int y1, int x2, int y2);

	// the input image (per-pixel foreground probabilities)
	pipeline::Input<Image>              _image;

	// an optional image to compute the potts term
	pipeline::Input<Image>              _pottsImage;

	// the paramemters (potts weight, neighborhood, ...)
	pipeline::Input<GraphCutParameters> _parameters;

	// the binary segmentation result
	pipeline::Output<Image>             _segmentation;

	// the energy of the result
	pipeline::Output<double>            _energy;

	// instantiation of graph
	graph_type _graph;

	bool _imageChanged;

	bool _gcParametersChanged;

	bool _pottsImageChanged;

	// indicates that the terminal weights need to be reset
	bool _setTerminalWeights;

	// indicates  that the edge weights need to be reset
	bool _setEdges;

	// size of the current graph
	int _graphWidth;
	int _graphHeight;

	// reuse previous solution
	bool _warmStart;

	// remember the previous parameters
	GraphCutParameters _prevParameters;

	// segmentation data
	vigra::MultiArray<2, float> _segmentationData;
};

#endif // IMAGEPROCESSING_GRAPH_CUT_H__

