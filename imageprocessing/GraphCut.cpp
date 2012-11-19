#include <math.h>
#include <vigra/basicimage.hxx>

#include "GraphCut.h"

logger::LogChannel graphcutlog("graphcutlog", "[GraphCut] ");

GraphCut::GraphCut() :
		_segmentation(boost::make_shared<Image>()),
		_energy(0),
		_graph(0, 0),
		_imageChanged(true),
		_gcParametersChanged(true),
		_pottsImageChanged(true),
		_graphWidth(0),
		_graphHeight(0),
		_warmStart(false) {

	registerInput(_image,"image");
	registerInput(_parameters,"parameters");
	registerInput(_pottsImage, "potts image");

	registerOutput(_segmentation, "segmentation");
	registerOutput(_energy, "energy");

	// register for input modification signals
	_image.registerBackwardCallback(&GraphCut::onModifiedImage, this);
	_parameters.registerBackwardCallback(&GraphCut::onModifiedGCParameters, this);
	_pottsImage.registerBackwardCallback(&GraphCut::onModifiedPottsImage, this);
}

void
GraphCut::onModifiedImage(const pipeline::Modified&) {

	LOG_DEBUG(graphcutlog) << "image modified!" << std::endl;

	// send the 'modified' signal upstream
	_imageChanged = true;
}

void
GraphCut::onModifiedPottsImage(const pipeline::Modified&) {

	LOG_DEBUG(graphcutlog) << "potts image modified!" << std::endl;

	// send the 'modified' signal upstream
	_pottsImageChanged = true;
}

void
GraphCut::onModifiedGCParameters(const pipeline::Modified&) {

	LOG_DEBUG(graphcutlog) << "parameters modified!" << std::endl;

	// send the 'modified' signal upstream
	_gcParametersChanged = true;
}

void
GraphCut::updateOutputs() {

	_setTerminalWeights = false;
	_setEdges = false;

	if (_imageChanged)
		_setTerminalWeights = true;

	if (_pottsImageChanged)
		_setEdges = true;

	if (_gcParametersChanged) {

		if (_parameters->pottsWeight != _prevParameters.pottsWeight)
			_setEdges = true;

		if (_parameters->contrastWeight != _prevParameters.contrastWeight)
			_setEdges = true;

		if (_parameters->contrastSigma != _prevParameters.contrastSigma)
			_setEdges = true;

		if (_parameters->foregroundPrior != _prevParameters.foregroundPrior)
			_setTerminalWeights = true;
	}

	doMaxFlow();

	_imageChanged = false;
	_gcParametersChanged = false;
	_pottsImageChanged = false;

	// try to perform a warm start on the next invocation
	_warmStart = true;

	// remember the current settings
	_prevParameters = *_parameters;
}

void
GraphCut::doMaxFlow() {

	// check if there was a change that requires recreation of the graph
	if (_graphWidth != _image->width() || _graphHeight != _image->height() ||
	    _parameters->eightNeighborhood != _prevParameters.eightNeighborhood ||
		_parameters->foregroundPrior == 0.0 || _parameters->foregroundPrior == 1.0 ||
		_prevParameters.foregroundPrior == 0.0 || _prevParameters.foregroundPrior == 1.0) {

		LOG_DEBUG(graphcutlog) << "(re)creating graph" << std::endl;

		_graph.reset();

		_setTerminalWeights = true;
		_setEdges = true;
		_warmStart = false;

		for (int i = 0; i < _image->width()*_image->height(); i++)
			_graph.add_node();

		_graphWidth = _image->width();
		_graphHeight = _image->height();
	}

	// add terminal weights (source and sink)
	if (_setTerminalWeights) {

		LOG_DEBUG(graphcutlog) << "setting terminal weights..." << std::endl;

		setTerminalWeights();

		_setTerminalWeights = false;
	}

	// adding edges between neighbors
	if (_setEdges) {

		LOG_DEBUG(graphcutlog) << "setting edge weights..." << std::endl;

		setEdgeWeights();

		_setEdges = false;
	}

	LOG_DEBUG(graphcutlog) << "finding max flow" << (_warmStart ? " (performing warm start)" : "") << "..." << std::endl;

	/* Althout this would be the desired call:
	 *
	 *  *_energy = _graph.maxflow(_warmStart);
	 *
	 * we can't do it, since there seem to be problems with numerical
	 * stabilities in Pushmeet's code. After a lot of warmstarts, the results
	 * are just wrong. This version, however, does at least reuse the graph (but
	 * not the search trees).
	 */
	*_energy = _graph.maxflow(false);

	// get the segmentation
	getSegmentation();
}

float
GraphCut::getCapacity(float probability, float foreground) {

	return -log(probability) - log(foreground);
}

int
GraphCut::getNodeId(int x, int y) {

	return x*_image->height() + y;
}

void
GraphCut::setTerminalWeights() {

	for (int x = 0; x < _image->width(); x++) {

		for (int y = 0; y < _image->height(); y++) {

			float value = (*_image)(x, y);

			unsigned int nodeId = getNodeId(x, y);

			_graph.edit_tweights_wt(
					nodeId,
					getCapacity(value, _parameters->foregroundPrior),
					getCapacity(1.0f - value, 1.0f - _parameters->foregroundPrior));
		}
	}
}

void
GraphCut::setEdgeWeights() {

	for (int x = 0; x < _image->width(); x++) {

		for (int y = 0; y < _image->height(); y++) {

			// adds edges to a node at x, y and assigns weights to those edges
			int nodeId = getNodeId(x, y);
			int neighborId;

			if (y-1 >= 0){

				neighborId = getNodeId(x, y-1);

				if (_warmStart)
					_graph.edit_edge_wt(nodeId, neighborId, getPairwiseCosts(x, y, x, y-1), getPairwiseCosts(x, y, x, y-1));
				else
					_graph.add_edge(nodeId, neighborId, getPairwiseCosts(x, y, x, y-1), getPairwiseCosts(x, y, x, y-1));
			}

			if (x-1 >= 0){

				neighborId = getNodeId(x-1, y);

				if (_warmStart)
					_graph.edit_edge_wt(nodeId, neighborId, getPairwiseCosts(x, y, x-1, y), getPairwiseCosts(x, y, x-1, y));
				else
					_graph.add_edge(nodeId, neighborId, getPairwiseCosts(x, y, x-1, y), getPairwiseCosts(x, y, x-1, y));
			}

			if (_parameters->eightNeighborhood) {

				if ( (x-1 >= 0) && (y-1 >= 0) ) {

					neighborId = getNodeId(x-1, y-1);

					if (_warmStart)
						_graph.edit_edge_wt(nodeId, neighborId, getPairwiseCosts(x, y, x-1, y-1), getPairwiseCosts(x, y, x-1, y-1));
					else
						_graph.add_edge(nodeId, neighborId, getPairwiseCosts(x, y, x-1, y-1), getPairwiseCosts(x, y, x-1, y-1));

				}

				if ( (y+1 < _image->height()) && (x-1 >= 0) ) {

					neighborId = getNodeId(x-1, y+1);

					if (_warmStart)
						_graph.edit_edge_wt(nodeId, neighborId, getPairwiseCosts(x, y, x-1, y+1), getPairwiseCosts(x, y, x-1, y+1));
					else
						_graph.add_edge(nodeId, neighborId, getPairwiseCosts(x, y, x-1, y+1), getPairwiseCosts(x, y, x-1, y+1));
				}
			}
		}
	}
}

void GraphCut::getSegmentation(){

	LOG_DEBUG(graphcutlog) << "resizing image to " << _image->shape() << std::endl;

	// adjust the size of the segmentation output to match the input image
	_segmentationData.reshape(_image->shape());
	*_segmentation = _segmentationData;

	for (int x = 0; x < _image->width(); x++){

		for (int y = 0; y < _image->height(); y++){

			unsigned int nodeId = getNodeId(x,y);

			if(_graph.what_segment(nodeId) == graph_type::SOURCE){

				(*_segmentation)(x, y) = 0;

			} else {

				(*_segmentation)(x, y) = 1;
			}
		}
	}
}

double GraphCut::getPairwiseCosts(int x1, int y1, int x2, int y2){

	// the distance between the two pixels
	double dist = sqrt(pow((x2 - x1), 2) + pow((y2 - y1), 2));

	double pottsTerm    = _parameters->pottsWeight/dist;
	double contrastTerm = 0.0;

	if (_pottsImage) {

		double g = (*_pottsImage)(x1, y1) - (*_pottsImage)(x2, y2);
		double e = exp ( -pow(g, 2) / (2.0 * pow(_parameters->contrastSigma, 2)));

		contrastTerm = _parameters->contrastWeight*e/dist;
	}
	return pottsTerm + contrastTerm;
}
