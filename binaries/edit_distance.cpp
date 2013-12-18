/**
 * edit distance main file. Initializes all objects, views, and visualizers.
 */

#include <iostream>
#include <gui/Window.h>
#include <gui/ZoomView.h>
#include <imageprocessing/gui/ImageStackView.h>
#include <imageprocessing/io/ImageStackDirectoryReader.h>
#include <pipeline/Process.h>
#include <pipeline/Value.h>
#include <sopnet/evaluation/TolerantEditDistance.h>
#include <util/ProgramOptions.h>
#include <util/Logger.h>

using namespace logger;

util::ProgramOption optionGroundTruth(
		util::_long_name        = "groundTruth",
		util::_description_text = "The ground truth image stack.",
		util::_default_value    = "groundtruth");

util::ProgramOption optionReconstruction(
		util::_long_name        = "reconstruction",
		util::_description_text = "The reconstruction image stack.",
		util::_default_value    = "reconstruction");

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

		// setup file readers and writers

		pipeline::Process<ImageStackDirectoryReader> groundTruthReader(optionGroundTruth.as<std::string>());
		pipeline::Process<ImageStackDirectoryReader> reconstructionReader(optionReconstruction.as<std::string>());

		// setup edit distance

		pipeline::Process<TolerantEditDistance> editDistance;

		// connect

		editDistance->setInput("ground truth", groundTruthReader->getOutput());
		editDistance->setInput("reconstruction", reconstructionReader->getOutput());

		// start GUI

		pipeline::Process<ImageStackView> stackView;
		pipeline::Process<gui::ZoomView>  zoomView;
		pipeline::Process<gui::Window>    window("edit distance");

		stackView->setInput(editDistance->getOutput("corrected reconstruction"));
		zoomView->setInput(stackView->getOutput());
		window->setInput(zoomView->getOutput());

		window->processEvents();

	} catch (Exception& e) {

		handleException(e, std::cerr);
	}
}


