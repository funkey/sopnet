/**
 * median filter main file. Initializes all objects, views, and visualizers.
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include <vigra/convolution.hxx>

#include <gui/ContainerView.h>
#include <gui/Slider.h>
#include <gui/VerticalPlacing.h>
#include <gui/Window.h>
#include <gui/ZoomView.h>
#include <imageprocessing/MedianFilter.h>
#include <imageprocessing/gui/ImageView.h>
#include <imageprocessing/io/ImageReader.h>
#include <imageprocessing/io/ImageWriter.h>
#include <pipeline/Process.h>
#include <pipeline/Value.h>
#include <util/ProgramOptions.h>
#include <util/Logger.h>

using namespace logger;

util::ProgramOption optionInFile(
		util::_long_name        = "in",
		util::_description_text = "The membrane image file to process.",
		util::_default_value    = "membrane.png");

util::ProgramOption optionOutFile(
		util::_long_name        = "out",
		util::_description_text = "The membrane file to write to.",
		util::_default_value    = "membrane_filtered.png");

util::ProgramOption optionRadius(
		util::_long_name        = "radius",
		util::_description_text = "The radius of the median filter to smooth the image.",
		util::_default_value    = 2);

util::ProgramOption optionHeadless(
		util::_long_name        = "headless",
		util::_description_text = "Create the filtered image and leave without gui");

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

		pipeline::Process<ImageReader> membraneReader(optionInFile.as<std::string>());

		// setup median radius slider

		pipeline::Process<gui::Slider<int> > radiusSlider("radius", 1, 10, optionRadius.as<int>());

		// setup median filter

		pipeline::Process<MedianFilter> medianFilter;

		medianFilter->setInput("radius", radiusSlider->getOutput("value"));
		medianFilter->setInput("image", membraneReader->getOutput());

		if (optionHeadless) {

			// setup image writer

			pipeline::Process<ImageWriter> imageWriter(optionOutFile.as<std::string>());

			imageWriter->setInput(medianFilter->getOutput());
			imageWriter->write();

		} else {

			// setup image views

			pipeline::Process<gui::ImageView> membraneView;
			pipeline::Process<gui::ImageView> medianView;

			membraneView->setInput(membraneReader->getOutput());
			medianView->setInput(medianFilter->getOutput());

			// setup container view

			pipeline::Process<gui::ContainerView<gui::VerticalPlacing> > container;

			container->addInput(membraneView->getOutput());
			container->addInput(radiusSlider->getOutput("painter"));
			container->addInput(medianView->getOutput());

			// setup zoom view

			pipeline::Process<gui::ZoomView> zoomView;

			zoomView->setInput(container->getOutput());

			// setup window

			pipeline::Process<gui::Window> window("membrane orientations");

			window->setInput(zoomView->getOutput());

			// start gui

			window->processEvents();
		}

	} catch (Exception& e) {

		handleException(e, std::cerr);
	}
}

