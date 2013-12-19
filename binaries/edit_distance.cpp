/**
 * edit distance main file. Initializes all objects, views, and visualizers.
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

		pipeline::Process<ImageStackView> gtView;
		pipeline::Process<ImageStackView> recView;
		pipeline::Process<ImageStackView> corRecView;
		pipeline::Process<ImageStackView> splitsView;
		pipeline::Process<ImageStackView> mergesView;
		pipeline::Process<gui::ZoomView>  zoomView;
		pipeline::Process<gui::Window>    window("edit distance");

		gtView->setInput(groundTruthReader->getOutput());
		recView->setInput(reconstructionReader->getOutput());
		corRecView->setInput(editDistance->getOutput("corrected reconstruction"));
		splitsView->setInput(editDistance->getOutput("splits"));
		mergesView->setInput(editDistance->getOutput("merges"));

		pipeline::Process<gui::NamedView> gtNamedView("ground truth");
		pipeline::Process<gui::NamedView> recNamedView("reconstruction");
		pipeline::Process<gui::NamedView> corRecNamedView("corrected");
		pipeline::Process<gui::NamedView> splitsNamedView("splits");
		pipeline::Process<gui::NamedView> mergesNamedView("merges");

		gtNamedView->setInput(gtView->getOutput());
		recNamedView->setInput(recView->getOutput());
		corRecNamedView->setInput(corRecView->getOutput());
		splitsNamedView->setInput(splitsView->getOutput());
		mergesNamedView->setInput(mergesView->getOutput());

		pipeline::Process<gui::ContainerView<gui::HorizontalPlacing> > container;
		container->setSpacing(10);
		container->setAlign(gui::HorizontalPlacing::Bottom);
		container->addInput(gtNamedView->getOutput());
		container->addInput(recNamedView->getOutput());
		container->addInput(corRecNamedView->getOutput());
		container->addInput(splitsNamedView->getOutput());
		container->addInput(mergesNamedView->getOutput());

		zoomView->setInput(container->getOutput());
		window->setInput(zoomView->getOutput());

		window->processEvents();

	} catch (Exception& e) {

		handleException(e, std::cerr);
	}
}


