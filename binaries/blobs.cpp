/**
 * sopnet main file. Initializes all objects, views, and visualizers.
 */

#include <iostream>
#include <string>

#include <pipeline/Process.h>
#include <util/exceptions.h>
#include <gui/ContainerView.h>
#include <gui/ExtractSurfaces.h>
#include <gui/HorizontalPlacing.h>
#include <gui/MeshView.h>
#include <gui/NamedView.h>
#include <gui/RotateView.h>
#include <gui/Slider.h>
#include <gui/Window.h>
#include <gui/ZoomView.h>
#include <imageprocessing/ImageExtractor.h>
#include <imageprocessing/SubStackSelector.h>
#include <imageprocessing/gui/ImageView.h>
#include <imageprocessing/gui/ImageStackView.h>
#include <imageprocessing/io/ImageStackDirectoryReader.h>
#include <sopnet/Sopnet.h>
#include <sopnet/evaluation/GroundTruthExtractor.h>
#include <sopnet/gui/NeuronsStackView.h>
#include <sopnet/gui/NeuronsView.h>
#include <sopnet/gui/SegmentsView.h>
#include <sopnet/io/IdMapCreator.h>
#include <sopnet/io/NeuronsImageWriter.h>
#include <sopnet/neurons/NeuronExtractor.h>
#include <sopnet/skeletons/FindInteriorPoints.h>
#include <sopnet/skeletons/FindSuperPixels.h>
#include <sopnet/skeletons/gui/SpheresView.h>
#include <util/ProgramOptions.h>
#include <util/SignalHandler.h>

using std::cout;
using std::endl;
using namespace gui;
using namespace logger;

util::ProgramOption optionMembranes(
		_long_name        = "membranes",
		_description_text = "The name of the directory containing the membrane maps.",
		_default_value    = "membranes");

class NeuronSelector : public pipeline::SimpleProcessNode<> {

public:

	NeuronSelector() {

		registerInput(_neurons, "neurons");
		registerInput(_selection, "selection");
		registerOutput(_neuron, "selected neuron");
	}

private:

	void updateOutputs() {

		_neuron = (*_neurons)[*_selection];
	}

	pipeline::Input<SegmentTrees> _neurons;
	pipeline::Input<unsigned int> _selection;
	pipeline::Output<SegmentTree> _neuron;
};

int main(int optionc, char** optionv) {

	// create a window
	pipeline::Process<gui::Window> window("splitmerge");

	try {

		/********
		 * INIT *
		 ********/

		// init command line parser
		util::ProgramOptions::init(optionc, optionv);

		// init logger
		LogManager::init();

		// init signal handler
		//util::SignalHandler::init();

		LOG_USER(out) << "[main] starting..." << std::endl;

		/*********
		 * SETUP *
		 *********/

		// create section readers
		pipeline::Process<ImageStackDirectoryReader> membranesReader(optionMembranes.as<std::string>());

		// create basic views
		pipeline::Process<ImageStackView> membranesView;
		membranesView->setInput(membranesReader->getOutput());

		// create a horizontal container
		pipeline::Process<ContainerView<HorizontalPlacing> > horizontalContainer;

		// create a smooth slider
		pipeline::Process<gui::Slider<double> > smoothSlider("smooth", 0.1, 20.0, 5.0);

		// find interior points in the membrane image stack
		pipeline::Process<FindInteriorPoints> findInteriorPoints;
		findInteriorPoints->setInput("membranes", membranesReader->getOutput());
		findInteriorPoints->setInput("smooth", smoothSlider->getOutput("value"));

		pipeline::Process<FindSuperPixels> findSuperPixels;
		findSuperPixels->setInput("boundary map", membranesReader->getOutput());
		findSuperPixels->setInput("seeds", findInteriorPoints->getOutput("spheres"));

		// image stack for found blobs
		pipeline::Process<ImageStackView> superPixelLabelView;
		superPixelLabelView->setInput(findSuperPixels->getOutput());

		// marching cubes
		pipeline::Process<ExtractSurfaces> extractSurfaces;
		extractSurfaces->setInput(findSuperPixels->getOutput());

		// mesh view for blobs
		pipeline::Process<MeshView> meshView;
		meshView->setInput(extractSurfaces->getOutput());

		// rotate view for meshes
		pipeline::Process<gui::RotateView> meshRotateView;
		meshRotateView->setInput(meshView->getOutput());

		// add to horizontal container
		horizontalContainer->addInput(membranesView->getOutput());
		horizontalContainer->addInput(smoothSlider->getOutput("painter"));
		horizontalContainer->addInput(superPixelLabelView->getOutput());
		horizontalContainer->addInput(meshRotateView->getOutput());

		// create a zoom view
		pipeline::Process<gui::ZoomView> zoomView(true);
		zoomView->setInput(horizontalContainer->getOutput());

		// show window
		window->setInput(zoomView->getOutput());
		window->processEvents();

		LOG_USER(out) << "[main] exiting..." << std::endl;

	} catch (Exception& e) {

		handleException(e, std::cerr);
	}
}

