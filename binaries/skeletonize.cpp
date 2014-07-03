/**
 * sopnet main file. Initializes all objects, views, and visualizers.
 */

#include <iostream>
#include <string>

#include <pipeline/Process.h>
#include <util/exceptions.h>
#include <gui/ContainerView.h>
#include <gui/HorizontalPlacing.h>
#include <gui/NamedView.h>
#include <gui/RotateView.h>
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
#include <sopnet/skeletons/FindSpheres.h>
#include <sopnet/skeletons/gui/SpheresView.h>
#include <util/ProgramOptions.h>
#include <util/SignalHandler.h>

using std::cout;
using std::endl;
using namespace gui;
using namespace logger;

util::ProgramOption optionRaw(
		_long_name        = "raw",
		_description_text = "The name of the directory containing the raw sections.",
		_default_value    = "raw");

util::ProgramOption optionNeurons(
		_long_name        = "neurons",
		_description_text = "The name of the directory containing the neuron ids.",
		_default_value    = "groundtruth");

util::ProgramOption optionShowNeurons(
		_long_name        = "showNeurons",
		_description_text = "Show a 3D view for each neuron.");

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
		pipeline::Process<ImageStackDirectoryReader> groundTruthReader(optionNeurons.as<std::string>());
		pipeline::Process<ImageStackDirectoryReader> rawReader(optionRaw.as<std::string>());

		// create ground-truth extractor
		pipeline::Process<GroundTruthExtractor> groundTruthExtractor(-1, -1, false);
		groundTruthExtractor->setInput(groundTruthReader->getOutput());

		// create neurons extractor
		pipeline::Process<NeuronExtractor> neuronsExtractor;
		neuronsExtractor->setInput(groundTruthExtractor->getOutput());

		// create basic views
		pipeline::Process<NeuronsStackView>  groundTruthView;
		groundTruthView->setInput(neuronsExtractor->getOutput());
		pipeline::Process<ImageStackView>    rawView;
		rawView->setInput(rawReader->getOutput());

		// create overlay container for these views
		pipeline::Process<ContainerView<OverlayPlacing> > overlay;
		overlay->addInput(groundTruthView->getOutput());
		overlay->addInput(rawView->getOutput());

		// create a horizontal container
		pipeline::Process<ContainerView<HorizontalPlacing> > horizontalContainer;
		horizontalContainer->addInput(overlay->getOutput());

		// create a vertical container
		pipeline::Process<ContainerView<VerticalPlacing> > verticalContainer;
		verticalContainer->addInput(horizontalContainer->getOutput());

		boost::shared_ptr<pipeline::ProcessNode> neuronsView;
		if (optionShowNeurons) {

			neuronsView = boost::make_shared<NeuronsView>();
			boost::shared_ptr<NamedView>   namedView   = boost::make_shared<NamedView>("Whole Neurons:");

			neuronsView->setInput(neuronsExtractor->getOutput());
			namedView->setInput(neuronsView->getOutput("container"));

			horizontalContainer->addInput(namedView->getOutput());

			pipeline::Process<NeuronSelector> selector;
			selector->setInput("neurons", neuronsExtractor->getOutput());
			selector->setInput("selection", neuronsView->getOutput("current neuron"));

			pipeline::Process<SegmentsView> neuronView;
			neuronView->setInput(selector->getOutput());

			pipeline::Process<FindSpheres> findSpheres;
			findSpheres->setInput(selector->getOutput());

			pipeline::Process<SpheresView> spheresView;
			spheresView->setInput(findSpheres->getOutput());

			pipeline::Process<ContainerView<OverlayPlacing> > neuronSpheresView;
			neuronSpheresView->addInput(neuronView->getOutput());
			neuronSpheresView->addInput(spheresView->getOutput());

			verticalContainer->addInput(neuronSpheresView->getOutput());
		}

		// create a zoom view
		pipeline::Process<gui::ZoomView> zoomView(true);
		zoomView->setInput(verticalContainer->getOutput());

		// show window
		window->setInput(zoomView->getOutput());
		window->processEvents();

		LOG_USER(out) << "[main] exiting..." << std::endl;

	} catch (Exception& e) {

		handleException(e, std::cerr);
	}
}
