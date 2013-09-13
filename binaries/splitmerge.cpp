/**
 * sopnet main file. Initializes all objects, views, and visualizers.
 */

#include <iostream>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

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
#include <sopnet/io/IdMapCreator.h>
#include <sopnet/io/NeuronsImageWriter.h>
#include <sopnet/neurons/NeuronExtractor.h>
#include <sopnet/segments/SplitMerge.h>
#include <util/ProgramOptions.h>
#include <util/SignalHandler.h>

using std::cout;
using std::endl;
using namespace gui;
using namespace logger;

int main(int optionc, char** optionv) {


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
		pipeline::Process<ImageStackDirectoryReader> groundTruthReader("./groundtruth/");
		pipeline::Process<ImageStackDirectoryReader> rawReader("./raw/");

		// create ground-truth extractor
		pipeline::Process<GroundTruthExtractor> groundTruthExtractor;
		groundTruthExtractor->setInput(groundTruthReader->getOutput());

		// create split-merge tool
		pipeline::Process<SplitMerge> splitMerge;
		splitMerge->setInput("initial segments", groundTruthExtractor->getOutput());

		// create neurons extractor
		pipeline::Process<NeuronExtractor> neuronsExtractor;
		neuronsExtractor->setInput(splitMerge->getOutput("segments"));

		// create basic views
		pipeline::Process<NeuronsStackView>  groundTruthView;
		groundTruthView->setInput(neuronsExtractor->getOutput());
		pipeline::Process<ImageStackView>    rawView;
		rawView->setInput(rawReader->getOutput());
		splitMerge->setInput("section", rawView->getOutput("section"));

		// create overlay container for these views
		pipeline::Process<ContainerView<OverlayPlacing> > container;
		container->addInput(splitMerge->getOutput("painter"));
		container->addInput(groundTruthView->getOutput());
		container->addInput(rawView->getOutput());

		// create a zoom view
		pipeline::Process<gui::ZoomView> zoomView(true);
		zoomView->setInput(container->getOutput());

		// create a window
		pipeline::Process<gui::Window> window("splitmerge");
		window->setInput(zoomView->getOutput());

		window->processEvents();

		LOG_USER(out) << "[main] exiting..." << std::endl;

	} catch (Exception& e) {

		handleException(e, std::cerr);
	}
}
