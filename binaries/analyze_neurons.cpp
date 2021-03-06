/**
 * sopnet main file. Initializes all objects, views, and visualizers.
 */

#include <iostream>
#include <string>

#include <pipeline/Process.h>
#include <util/exceptions.h>
#include <gui/ContainerView.h>
#include <gui/ExtractSurface.h>
#include <gui/HorizontalPlacing.h>
#include <gui/MeshView.h>
#include <gui/NamedView.h>
#include <gui/RotateView.h>
#include <gui/Slider.h>
#include <gui/Switch.h>
#include <gui/VerticalPlacing.h>
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
#include <sopnet/skeletons/ConvexDecomposition.h>
#include <sopnet/skeletons/FindSpheres.h>
#include <sopnet/skeletons/FindSuperPixels.h>
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
		pipeline::Process<GroundTruthExtractor> groundTruthExtractor;
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
			findSpheres->setInput("neuron", selector->getOutput());

			pipeline::Process<gui::Slider<double> > smoothSlider("smooth", 0.0, 20.0, 10.0);
			findSpheres->setInput("smooth", smoothSlider->getOutput("value"));

			pipeline::Process<SpheresView> spheresView;
			spheresView->setInput(findSpheres->getOutput("spheres"));

			pipeline::Process<ImageStackView> maxDistanceView;
			maxDistanceView->setInput(findSpheres->getOutput("distances"));

			pipeline::Process<ContainerView<OverlayPlacing> > neuronSpheresView;
			neuronSpheresView->addInput(neuronView->getOutput());
			neuronSpheresView->addInput(spheresView->getOutput());

			pipeline::Process<gui::RotateView> rotateView;
			rotateView->setInput(neuronSpheresView->getOutput());

			pipeline::Process<FindSuperPixels> findSuperPixels;
			findSuperPixels->setInput("boundary map", findSpheres->getOutput("distances"));
			findSuperPixels->setInput("seeds", findSpheres->getOutput("spheres"));

			pipeline::Process<ImageStackView> superPixelLabelView;
			superPixelLabelView->setInput(findSuperPixels->getOutput());

			pipeline::Process<ExtractSurface> extractSurface;
			extractSurface->setInput(findSuperPixels->getOutput());

			pipeline::Process<ConvexDecomposition> convexDecomposition;
			convexDecomposition->setInput(extractSurface->getOutput());

			// convexification parameters
			pipeline::Process<gui::Slider<double> >       compacityWeightSlider(      "compacity weight",        0.0, 1.0, 0.5);
			pipeline::Process<gui::Slider<double> >       volumeWeightSlider(         "volume weight",           0.0, 1.0, 0.0);
			pipeline::Process<gui::Slider<double> >       connectDistanceSlider(      "connect distance",        0, 100, 30);
			pipeline::Process<gui::Slider<unsigned int> > minNumClustersSlider(       "min num clusters",        1, 100, 1);
			pipeline::Process<gui::Slider<unsigned int> > maxNumHullVerticesSlider(   "max num hull vertices",   10, 10000, 10000);
			pipeline::Process<gui::Slider<double> >       maxConcavitySlider(         "max concavity",           1, 1000, 450);
			pipeline::Process<gui::Slider<double> >       smallClusterThresholdSlider("small cluster threshold", 0.0, 1.0, 0.25);
			pipeline::Process<gui::Slider<unsigned int> > numTargetTrianglesSlider(   "num target triangles",    100, 10000, 3000);
			pipeline::Process<gui::Switch>                addExtraDistPointsSlider(   "add extra dist points",   true);
			pipeline::Process<gui::Switch>                addExtraFacesPointsSlider(  "add extra faces points",  true);

			convexDecomposition->setInput("compacity weight",        compacityWeightSlider->getOutput("value"));
			convexDecomposition->setInput("volume weight",           volumeWeightSlider->getOutput("value"));
			convexDecomposition->setInput("connect distance",        connectDistanceSlider->getOutput("value"));
			convexDecomposition->setInput("min num clusters",        minNumClustersSlider->getOutput("value"));
			convexDecomposition->setInput("max num hull vertices",   maxNumHullVerticesSlider->getOutput("value"));
			convexDecomposition->setInput("max concavity",           maxConcavitySlider->getOutput("value"));
			convexDecomposition->setInput("small cluster threshold", smallClusterThresholdSlider->getOutput("value"));
			convexDecomposition->setInput("num target triangles",    numTargetTrianglesSlider->getOutput("value"));
			convexDecomposition->setInput("add extra dist points",   addExtraDistPointsSlider->getOutput("value"));
			convexDecomposition->setInput("add extra faces points",  addExtraFacesPointsSlider->getOutput("value"));

			// convexification parameters gui
			pipeline::Process<gui::ContainerView<gui::VerticalPlacing> > convexificationGui;
			convexificationGui->addInput(compacityWeightSlider->getOutput("painter"));
			convexificationGui->addInput(volumeWeightSlider->getOutput("painter"));
			convexificationGui->addInput(connectDistanceSlider->getOutput("painter"));
			convexificationGui->addInput(minNumClustersSlider->getOutput("painter"));
			convexificationGui->addInput(maxNumHullVerticesSlider->getOutput("painter"));
			convexificationGui->addInput(maxConcavitySlider->getOutput("painter"));
			convexificationGui->addInput(smallClusterThresholdSlider->getOutput("painter"));
			convexificationGui->addInput(numTargetTrianglesSlider->getOutput("painter"));
			convexificationGui->addInput(addExtraDistPointsSlider->getOutput("painter"));
			convexificationGui->addInput(addExtraFacesPointsSlider->getOutput("painter"));

			pipeline::Process<MeshView> meshView;
			meshView->setInput(convexDecomposition->getOutput());

			pipeline::Process<gui::RotateView> meshRotateView;
			meshRotateView->setInput(meshView->getOutput());

			pipeline::Process<ContainerView<HorizontalPlacing> > resultView;
			resultView->addInput(maxDistanceView->getOutput());
			resultView->addInput(smoothSlider->getOutput("painter"));
			resultView->addInput(rotateView->getOutput());
			resultView->addInput(superPixelLabelView->getOutput());
			resultView->addInput(convexificationGui->getOutput());
			resultView->addInput(meshRotateView->getOutput());

			verticalContainer->addInput(resultView->getOutput());
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
