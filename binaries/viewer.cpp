/**
 * viewer main file. Initializes all objects, views, and visualizers.
 *
 * A simple image stack viewer.
 */

#include <iostream>
#include <gui/ContainerView.h>
#include <gui/OverlayPlacing.h>
#include <gui/Window.h>
#include <gui/ZoomView.h>
#include <imageprocessing/gui/ImageStackView.h>
#include <imageprocessing/io/ImageStackDirectoryReader.h>
#include <sopnet/evaluation/GroundTruthExtractor.h>
#include <sopnet/neurons/NeuronExtractor.h>
#include <sopnet/gui/NeuronsStackView.h>
#include <pipeline/Process.h>
#include <pipeline/Value.h>
#include <util/ProgramOptions.h>
#include <util/Logger.h>

using namespace logger;

util::ProgramOption optionStack(
		util::_long_name        = "stack",
		util::_description_text = "The image stack.",
		util::_default_value    = "stack",
		util::_is_positional    = true);

util::ProgramOption optionOverlay(
		util::_long_name        = "overlay",
		util::_description_text = "An optional overlay image stack with component ids.");

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

		pipeline::Process<ImageStackDirectoryReader> stackReader(optionStack.as<std::string>());

		// start GUI

		pipeline::Process<ImageStackView> stackView;
		pipeline::Process<gui::ZoomView>  zoomView;
		pipeline::Process<gui::Window>    window("edit distance");

		stackView->setInput(stackReader->getOutput());

		pipeline::Process<gui::ContainerView<gui::OverlayPlacing> > container;

		if (optionOverlay) {

			pipeline::Process<ImageStackDirectoryReader> overlayReader(optionOverlay.as<std::string>());
			pipeline::Process<GroundTruthExtractor>      overlayExtractor;
			pipeline::Process<NeuronExtractor>           componentExtractor;
			pipeline::Process<NeuronsStackView>          overlayView;

			overlayExtractor->setInput(overlayReader->getOutput());
			componentExtractor->setInput(overlayExtractor->getOutput());
			overlayView->setInput(componentExtractor->getOutput());

			container->addInput(overlayView->getOutput());
		}

		container->addInput(stackView->getOutput());

		zoomView->setInput(container->getOutput());
		window->setInput(zoomView->getOutput());

		window->processEvents();

	} catch (Exception& e) {

		handleException(e, std::cerr);
	}
}


