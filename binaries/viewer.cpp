/**
 * viewer main file. Initializes all objects, views, and visualizers.
 *
 * A simple image stack viewer.
 */

#include <iostream>
#include <gui/ContainerView.h>
#include <gui/HorizontalPlacing.h>
#include <gui/NamedView.h>
#include <gui/Window.h>
#include <gui/ZoomView.h>
#include <imageprocessing/gui/ImageStackView.h>
#include <imageprocessing/io/ImageStackDirectoryReader.h>
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

		pipeline::Process<gui::NamedView> stackNamedView("stack");

		stackNamedView->setInput(stackView->getOutput());

		pipeline::Process<gui::ContainerView<gui::HorizontalPlacing> > container;
		container->setSpacing(10);
		container->setAlign(gui::HorizontalPlacing::Bottom);
		container->addInput(stackNamedView->getOutput());

		zoomView->setInput(container->getOutput());
		window->setInput(zoomView->getOutput());

		window->processEvents();

	} catch (Exception& e) {

		handleException(e, std::cerr);
	}
}


