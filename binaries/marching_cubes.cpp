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
		pipeline::Process<ExtractSurfaces> extractSurfaces;
		extractSurfaces->setInput(rawReader->getOutput());

		// mesh view
		pipeline::Process<MeshView> meshView;
		meshView->setInput(extractSurfaces->getOutput());

		// rotate view
		pipeline::Process<gui::RotateView> rotateView;
		rotateView->setInput(meshView->getOutput());

		// create a zoom view
		pipeline::Process<gui::ZoomView> zoomView(true);
		zoomView->setInput(rotateView->getOutput());

		// show window
		window->setInput(zoomView->getOutput());
		window->processEvents();

		LOG_USER(out) << "[main] exiting..." << std::endl;

	} catch (Exception& e) {

		handleException(e, std::cerr);
	}
}

