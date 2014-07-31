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

int main(int optionc, char** optionv) {

	// create a window
	pipeline::Process<gui::Window> window("marching cubes");

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

		// create section reader
		pipeline::Process<ImageStackDirectoryReader> rawReader(optionRaw.as<std::string>());

		// marching cubes
		pipeline::Process<ExtractSurface> extractSurface;
		extractSurface->setInput(rawReader->getOutput());

		// convexification
		pipeline::Process<ConvexDecomposition> convexDecomposition;
		convexDecomposition->setInput(extractSurface->getOutput());

		// convexification parameters
		pipeline::Process<gui::Slider<double> >       compacityWeightSlider(      "compacity weight",        0.0, 1.0, 0.0001);
		pipeline::Process<gui::Slider<double> >       volumeWeightSlider(         "volume weight",           0.0, 1.0, 0.0);
		pipeline::Process<gui::Slider<double> >       connectDistanceSlider(      "connect distance",        0, 100, 30);
		pipeline::Process<gui::Slider<unsigned int> > minNumClustersSlider(       "min num clusters",        1, 100, 1);
		pipeline::Process<gui::Slider<unsigned int> > maxNumHullVerticesSlider(   "max num hull vertices",   10, 10000, 1000);
		pipeline::Process<gui::Slider<double> >       maxConcavitySlider(         "max concavity",           1, 1000, 200);
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

		// mesh view
		pipeline::Process<MeshView> meshView;
		meshView->setInput(convexDecomposition->getOutput());

		// rotate view
		pipeline::Process<gui::RotateView> rotateView;
		rotateView->setInput(meshView->getOutput());

		// horizontal container
		pipeline::Process<gui::ContainerView<gui::HorizontalPlacing> > container;
		container->addInput(convexificationGui->getOutput());
		container->addInput(rotateView->getOutput());

		// create a zoom view
		pipeline::Process<gui::ZoomView> zoomView(true);
		zoomView->setInput(container->getOutput());

		// show window
		window->setInput(zoomView->getOutput());
		window->processEvents();

		LOG_USER(out) << "[main] exiting..." << std::endl;

	} catch (Exception& e) {

		handleException(e, std::cerr);
	}
}

