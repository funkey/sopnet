#ifndef IMAGEPROCESSING_GRAPH_CUT_H__
#define IMAGEPROCESSING_GRAPH_CUT_H__

#include <external/maxflow/graph.h>

#include <imageprocessing/Image.h>
#include <pipeline.h>
#include <util/Logger.h>

#include "GraphCutParameters.h"


class GraphCut : public pipeline::ProcessNode {

	typedef Graph<float,float,float> GraphType;

public:

	GraphCut();

private:

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

	// signal to indicate that output is out-of-date
	signals::Slot<pipeline::Modified>   _modified;

	// signal to ask the sink to update inputs
	signals::Slot<pipeline::Update>     _updateImage;
	signals::Slot<pipeline::Update>     _updateGCParameters;
	signals::Slot<pipeline::Update>     _updatePottsImage;

	// signal to indicate that ouput is up-to-date
	signals::Slot<pipeline::Updated>    _updated;

	// instantiation of graph
	GraphType                           _graph;

	bool _imageChanged;
	bool _gcParametersChanged;
	bool _pottsImageChanged;
	bool _setTerminalWeights;

	int _graphHeight;					// number of nodes in the y direction
	int _graphWidth;					// number of nodes in the x direction
										// these are used to check if the dimensions of the existing graph matches those of the image

	vigra::MultiArray<2, float> _segmentationData;

	void onModifiedImage(const pipeline::Modified& signal);
	void onModifiedGCParameters(const pipeline::Modified& signal);
	void onModifiedPottsImage(const pipeline::Modified& signal);
	void onUpdatedGCParameters(const pipeline::Updated& signal);
	void onUpdatedImage(const pipeline::Updated& signal);
	void onUpdatedPottsImage(const pipeline::Updated& signal);
	void onUpdateGraphCut(const pipeline::Update& signal);

	void doMaxFlow();
	void assignEdges();
	void setTerminalWeights();
	void getSegmentation();

	float getCapacity(float probability, float foreground);

	int getNodeId(int x, int y);

	bool imageDimensionsDifferGraph();

	double getPairwiseCosts(int x1, int y1, int x2, int y2);
};

#endif // IMAGEPROCESSING_GRAPH_CUT_H__

