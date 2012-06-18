#include <math.h>
#include <vigra/basicimage.hxx>

#include "GraphCut.h"

logger::LogChannel graphCutLog("graphCutLog", "[GraphCut] ");

GraphCut::GraphCut() : _graph(0,0),
		_imageChanged(false),
		_gcParametersChanged(false),
		_pottsImageChanged(false),
		_setTerminalWeights(true),
		_graphHeight(0),
		_graphWidth(0){

	registerInput(_image,"image");
	registerInput(_parameters,"parameters");
	registerInput(_pottsImage, "potts image");

	registerOutput(_segmentation, "segmentation");
	registerOutput(_energy, "energy");

	// receiving modified signal from the inputs
	_image.registerBackwardCallback(&GraphCut::onModifiedImage, this);
	_parameters.registerBackwardCallback(&GraphCut::onModifiedGCParameters, this);
	_pottsImage.registerBackwardCallback(&GraphCut::onModifiedPottsImage, this);

	// receiving the updated signal from the inputs
	_image.registerBackwardCallback(&GraphCut::onUpdatedImage, this);
	_parameters.registerBackwardCallback(&GraphCut::onUpdatedGCParameters, this);
	_pottsImage.registerBackwardCallback(&GraphCut::onUpdatedPottsImage, this);

	// receiving the 'update' signal from the outputs
	_segmentation.registerForwardCallback(&GraphCut::onUpdateGraphCut, this);
	_energy.registerForwardCallback(&GraphCut::onUpdateGraphCut, this);

	// sending the update signal for the inputs
	_image.registerBackwardSlot(_updateImage);
	_parameters.registerBackwardSlot(_updateGCParameters);
	_pottsImage.registerBackwardSlot(_updatePottsImage);

	// sending the modified signal for the outputs
	_segmentation.registerForwardSlot(_modified);
	_energy.registerForwardSlot(_modified);

	// sending the updated signal for the outputs
	_segmentation.registerForwardSlot(_updated);
	_energy.registerForwardSlot(_updated);

}


// define relevant signal class and implement method

void GraphCut::onModifiedImage(const pipeline::Modified& signal){
	// called when "modified" signal is received for image
	LOG_DEBUG(graphCutLog) << "Input modified signal received: image modified!" << std::endl;

	// send the 'modified' signal upstream
	_imageChanged = true;
	_modified();
}

void GraphCut::onModifiedPottsImage(const pipeline::Modified& signal){
	// called when "modified" signal is received for potts image
	LOG_USER(graphCutLog) << "Input modified signal received: potts image modified!" << std::endl;

	// send the 'modified' signal upstream
	_pottsImageChanged = true;
	_modified();
}

void GraphCut::onModifiedGCParameters(const pipeline::Modified& signal){
	// called when "modified" signal is received for parameters
	LOG_DEBUG(graphCutLog) << "Input modified signal received: GC Parameters modified!" << std::endl;

	// send the 'modified' signal upstream
	_gcParametersChanged = true;
	_modified();
}

void GraphCut::onUpdateGraphCut(const pipeline::Update& signal){
	// send update signal downstream
	if(_imageChanged){
		LOG_DEBUG(graphCutLog) << "Received 'update image' signal. Forwarding downstream" << std::endl;
		_updateImage();
	}

	if(_gcParametersChanged){
		LOG_DEBUG(graphCutLog) << "Received 'update GC parameters'. Forwarding downstream" << std::endl;
		_updateGCParameters();
	}

	if(_pottsImageChanged){
			LOG_DEBUG(graphCutLog) << "Received 'update potts image'. Forwarding downstream" << std::endl;
			_updatePottsImage();
		}
}

void GraphCut::onUpdatedImage(const pipeline::Updated& signal){
	// called when the "updated" signal is received for image
	LOG_DEBUG(graphCutLog) << "Input updated: image" << std::endl;
	_setTerminalWeights = true;
	if (_imageChanged) {
		_imageChanged = false;
		// send the 'updated' message upstream
		if(!_gcParametersChanged && !_pottsImageChanged){
			// the updated inputs are now available
			doMaxFlow();
			LOG_DEBUG(graphCutLog) << "Updated GraphCut output" << std::endl;
			_updated();
		}
	}
}

void GraphCut::onUpdatedGCParameters(const pipeline::Updated& signal){
	// called when the "updated" signal is received for parameters
	LOG_DEBUG(graphCutLog) << "Input updated: GC parameters" << std::endl;
	// send the 'updated' message upstream
	if(_gcParametersChanged){
		_gcParametersChanged = false;
		if(!_imageChanged && !_pottsImageChanged){
			// the updated inputs are now available
			doMaxFlow();
			LOG_DEBUG(graphCutLog) << "Updated GraphCut output" << std::endl;
			_updated();
		}
	}
}

void GraphCut::onUpdatedPottsImage(const pipeline::Updated& signal){
	// called when the "updated" signal is received for potts image
	LOG_DEBUG(graphCutLog) << "Input updated: potts image" << std::endl;
	if(_pottsImageChanged){
		_pottsImageChanged = false;
		if(!_imageChanged && !_gcParametersChanged){
			// the updated inputs are now available
			doMaxFlow();
			LOG_DEBUG(graphCutLog) << "Updated GraphCut output" << std::endl;
			_updated();
		}
	}
}

void GraphCut::doMaxFlow(){
	// calculates the mincut/maxflow of the graph using the algorithm implemented by Kolmogorov and Boykov (2004)
	int thisNodeId = 0;
	int neighborId = 0;

	float value = 0.0;					// stores value of pixel
	int x = 0;
	int y = 0;
	bool reassignEdges = false;			// flag to see if the edges have to be reassigned

	// check if the size of the image is different from the number of nodes in the graph
	// if different, reset the graph to (0,0)
	// if(imageDimensionsDifferGraph()){  // temporarily disabled until the assignment of multiple edges and weights
										  // in external/maxflow is fixed
	if(true){                             // resetting the graph every time an input is modified (temporary feature until the above modification is fixed)
		LOG_DEBUG(graphCutLog) << "doMaxFlow: Graph reset to zero due to mismatch with image size!" << std::endl;
		_graph.reset();
		reassignEdges = true;
		_setTerminalWeights = true;
		// build the graph to fit the number of pixels in the image
		LOG_DEBUG(graphCutLog) << "Creating graph.." << std::endl;
		for (x = 0; x < _image->width(); x++){
			for (y = 0; y < _image->height(); y++){
				_graph.add_node();
				LOG_DEBUG(graphCutLog) << "Node: " << _graph.get_node_num() << " added to graph!" << std::endl;
			}
		}
		_graphHeight = y;
		_graphWidth = x;
		LOG_DEBUG(graphCutLog) << "doMaxFlow: Graph has height: " << _graphHeight << " and width: " << _graphWidth << std::endl;
		LOG_DEBUG(graphCutLog) << "doMaxFlow: Number of nodes: " << _graph.get_node_num() << std::endl;
	}
	// add terminal weights (source and sink)
	if(_setTerminalWeights){					// obsolete if the graph is reset every time an input has changed
		LOG_DEBUG(graphCutLog) << "doMaxFlow: Assigning terminal weights.." << std::endl;
		setTerminalWeights();
		_setTerminalWeights = false;            // terminal weights are set only if the image is modified
	}

	// adding edges between neighbors
	// only if the size of the image has changed
	if(reassignEdges){						    // at the moment, the edges between nodes are reassigned every time, since the graph is reset every time
		LOG_DEBUG(graphCutLog) << "doMaxFlow: Assigning edges.." << std::endl;
		assignEdges();
	}

	LOG_DEBUG(graphCutLog) << "doMaxFlow: Calculating energy..." << std::endl;
	*_energy = _graph.maxflow();
	LOG_DEBUG(graphCutLog) << "doMaxFlow: Energy calculated." << std::endl;

	// adjusting the size of the segmentation output to match the input image
	_segmentationData.reshape(_image->shape());
	*_segmentation = _segmentationData;

	// get the segmentation
	getSegmentation();
}

bool GraphCut::imageDimensionsDifferGraph(){
	// checks if the height and the width of the graph are equal to that of the image
	if(_image->height()!= _graphHeight){
		return true;
	}
	else if (_image->width() != _graphWidth) {
		return true;
	}else
		return false;
}

float GraphCut::getCapacity(float probability, float foreground){
	// calculates the capacity based on the probability at the node
	return -log(probability) - log(foreground);
}

int GraphCut::getNodeId(int x, int y){
	// calculates the ID for a node given its coordinates
	return x * _image->height() + y;
}

void GraphCut::setTerminalWeights(){
	float value;
	int thisNodeId;
	for (int x = 0; x < _image->width(); x++){
		for (int y = 0; y < _image->height(); y++){
			value = (*_image)(x, y);
			thisNodeId = getNodeId( x, y);
			_graph.add_tweights(thisNodeId, getCapacity(value, _parameters->foregroundPrior), getCapacity(1.0f - value, 1.0f - _parameters->foregroundPrior));
		}
	}
}

void GraphCut::getSegmentation(){
	int thisNodeId;
	for (int x = 0; x < _image->width(); x++){
		for (int y = 0; y < _image->height(); y++){
			thisNodeId = getNodeId(x,y);
			if(_graph.what_segment(thisNodeId) == GraphType::SOURCE){
				// if the node is of type source, set value to 0, in the segmented image (output)
				(*_segmentation)(x, y) = 0;

			} else {
				// if the node is of type sink, set value to 1, in the segmented image (output)
				(*_segmentation)(x, y) = 1;
			}
		}
	}
}

double GraphCut::getPairwiseCosts(int x1, int y1, int x2, int y2){

	double dist = sqrt(pow((x2 - x1), 2) + pow((y2 - y1), 2)); // calculating the distance between two pixels

	double pottsTerm    = _parameters->pottsWeight/dist;
	double contrastTerm = 0.0;

	if (_pottsImage) {

		double g = (*_pottsImage)(x1, y1) - (*_pottsImage)(x2, y2);
		double e = exp ( -pow(g, 2) / (2.0 * pow(_parameters->contrastSigma, 2)));

		contrastTerm = _parameters->contrastWeight*e/dist;
	}
	return pottsTerm + contrastTerm;
}

void GraphCut::assignEdges() {
	for (int x = 0; x < _image->width(); x++){
		for (int y = 0; y < _image->height(); y++){
			// adds edges to a node at x, y and assigns weights to those edges
			int thisNodeId = getNodeId(x, y);
			int neighborId;

			// with pottsImage
			// get neighbors (2 neighbors initially for fourNeighborhood)
			// add a pair of edges for each neighbor discovered earlier
			if (y-1 >= 0){
				neighborId = getNodeId(x, y-1);
				_graph.add_edge(thisNodeId, neighborId, getPairwiseCosts(x, y, x, y-1), getPairwiseCosts(x, y, x, y-1));
			}
			if (x-1 >= 0){
				neighborId = getNodeId(x-1, y);
				_graph.add_edge(thisNodeId, neighborId, getPairwiseCosts(x, y, x-1, y), getPairwiseCosts(x, y, x-1, y));
			}
			if(_parameters->eightNeighborhood){
				// get the third neighbor if eightNeighborhood is enabled
				if( (x-1 >= 0) && (y-1 >= 0) ){
					neighborId = getNodeId(x-1, y-1);
					_graph.add_edge(thisNodeId, neighborId, getPairwiseCosts(x, y, x-1, y-1), getPairwiseCosts(x, y, x-1, y-1));
				}
				// get the 4th neighbor if eightNeighborhood is enabled
				if( (y+1 < _image->height()) && (x-1 >= 0) ){
					neighborId = getNodeId(x-1, y+1);
					_graph.add_edge(thisNodeId, neighborId, getPairwiseCosts(x, y, x-1, y+1), getPairwiseCosts(x, y, x-1, y+1));
				}
			}
		}
	}
}
