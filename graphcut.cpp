/**
 * graphcut main file. Initializes all objects, views, and visualizers.
 */

#include <iostream>
#include <string>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>
#include <boost/progress.hpp>

#include <util/exceptions.h>
#include <gui/ContainerView.h>
#include <gui/HorizontalPlacing.h>
#include <gui/ImageView.h>
#include <gui/NamedView.h>
#include <gui/RotateView.h>
#include <gui/Window.h>
#include <gui/ZoomView.h>
#include <imageprocessing/GraphCut.h>
#include <imageprocessing/GraphCutSequence.h>
#include <imageprocessing/ImageExtractor.h>
#include <imageprocessing/SubStackSelector.h>
#include <imageprocessing/gui/ImageStackView.h>
#include <imageprocessing/gui/GraphCutDialog.h>
#include <imageprocessing/io/ImageStackHdf5Reader.h>
#include <imageprocessing/io/ImageStackDirectoryReader.h>
#include <util/ProgramOptions.h>

using std::cout;
using std::endl;
using namespace gui;
using namespace logger;

util::ProgramOption optionProjectName(
		_long_name        = "project",
		_short_name       = "p",
		_description_text = "The HDF5 project file.");

util::ProgramOption optionCreateSequence(
		_long_name        = "createSequence",
		_short_name       = "s",
		_description_text = "Create a sequence of graph-cuts.");

util::ProgramOption optionFirstSection(
		_module           = "graphcut",
		_long_name        = "firstSection",
		_description_text = "The number of the first section to process.",
		_default_value    = 0);

util::ProgramOption optionLastSection(
		_module           = "graphcut",
		_long_name        = "lastSection",
		_description_text = "The number of the last section to process. If set to -1, all sections after <firstSection> will be used.",
		_default_value    = -1);

void handleException(boost::exception& e) {

	LOG_ERROR(out) << "[window thread] caught exception: ";

	if (boost::get_error_info<error_message>(e))
		LOG_ERROR(out) << *boost::get_error_info<error_message>(e);

	if (boost::get_error_info<stack_trace>(e))
		LOG_ERROR(out) << *boost::get_error_info<stack_trace>(e);

	LOG_ERROR(out) << std::endl;

	LOG_ERROR(out) << "[window thread] details: " << std::endl
	               << boost::diagnostic_information(e)
	               << std::endl;

	exit(-1);
}

void processEvents(boost::shared_ptr<gui::Window> window) {

	LOG_USER(out) << " started as " << window->getCaption() << " at " << window.get() << std::endl;

	while (!window->closed()) {

		try {

			usleep(100);
			window->processEvents();

		} catch (boost::exception& e) {

			handleException(e);
		}
	}

	LOG_USER(out) << "[window thread] releasing shared pointer to window" << std::endl;

	LOG_USER(out) << "[window thread] quitting" << std::endl;
}

int main(int optionc, char** optionv) {

	try {

		/********
		 * INIT *
		 ********/

		// init command line parser
		util::ProgramOptions::init(optionc, optionv);

		// init logger
		LogManager::init();

		LOG_USER(out) << "[main] starting..." << std::endl;

		/*********
		 * SETUP *
		 *********/

		// create a window
		boost::shared_ptr<gui::Window> window = boost::make_shared<gui::Window>("sopnet");
		window->processEvents();

		// create a zoom view for this window
		boost::shared_ptr<gui::ZoomView> zoomView = boost::make_shared<gui::ZoomView>();
		window->setInput(zoomView->getOutput());

		// create main container view
		boost::shared_ptr<ContainerView<VerticalPlacing> > mainContainer = boost::make_shared<ContainerView<VerticalPlacing> >();

		// create image views
		boost::shared_ptr<ImageStackView> membranesView = boost::make_shared<ImageStackView>();

		// create graphcut dialog
		boost::shared_ptr<GraphCutDialog> graphcutDialog = boost::make_shared<GraphCutDialog>();

		// connect them to the window via the zoom view
		mainContainer->addInput(membranesView->getOutput("painter"));
		mainContainer->addInput(graphcutDialog->getOutput("painter"));
		zoomView->setInput(mainContainer->getOutput());

		// create section readers
		boost::shared_ptr<pipeline::ProcessNode> membranesReader;

		// create image stack readers
		if (!optionProjectName) {

			// if no project filename was given, try to read from default
			// directoryies
			membranesReader = boost::make_shared<ImageStackDirectoryReader>("./membranes/");

		} else {

			// get the project filename
			std::string projectFilename = optionProjectName;

			// try to read from project hdf5 file
			membranesReader = boost::make_shared<ImageStackHdf5Reader>(projectFilename, "vncstack", "membranes");
		}

		// select a substack, if options are set
		if (optionFirstSection || optionLastSection) {

			int firstSection = optionFirstSection;
			int lastSection  = optionLastSection;

			// create section selector
			boost::shared_ptr<SubStackSelector> membranesSelector = boost::make_shared<SubStackSelector>(firstSection, lastSection);

			// set its input to the output of the section reader
			membranesSelector->setInput(membranesReader->getOutput());

			// sneakily pretend the selector is the readers
			membranesReader = membranesSelector;
		}

		// set input for image stack views
		membranesView->setInput(membranesReader->getOutput());

		// create graphcut
		boost::shared_ptr<GraphCut> graphcut = boost::make_shared<GraphCut>();

		// set input to graphcut
		graphcut->setInput("image", membranesView->getOutput("current image"));
		graphcut->setInput("potts image", membranesView->getOutput("current image"));
		graphcut->setInput("parameters", graphcutDialog->getOutput("parameters"));

		// create image view for graphcut result
		boost::shared_ptr<ImageView> segmentationView = boost::make_shared<ImageView>();

		// connect it to graphcut
		segmentationView->setInput(graphcut->getOutput("segmentation"));

		// add it to main container
		mainContainer->addInput(segmentationView->getOutput("painter"));

		if (optionCreateSequence) {

			boost::shared_ptr<GraphCutSequence> graphCutSequence = boost::make_shared<GraphCutSequence>();

			graphCutSequence->setInput(membranesReader->getOutput());

			graphCutSequence->createSequence();
		}

		while (!window->closed()) {

			window->processEvents();
			usleep(1000);
		}

		LOG_USER(out) << "[main] exiting..." << std::endl;

	} catch (Exception& e) {

		handleException(e);
	}
}
