/**
 * edit distance main file. Initializes all objects, views, and visualizers.
 */

#include <iostream>
#include <gui/ContainerView.h>
#include <gui/HorizontalPlacing.h>
#include <gui/NamedView.h>
#include <gui/NumberView.h>
#include <gui/Window.h>
#include <gui/ZoomView.h>
#include <imageprocessing/gui/ImageStackView.h>
#include <imageprocessing/io/ImageStackDirectoryReader.h>
#include <imageprocessing/io/ImageStackDirectoryWriter.h>
#include <pipeline/Process.h>
#include <pipeline/Value.h>
#include <sopnet/evaluation/TolerantEditDistance.h>
#include <sopnet/evaluation/TolerantEditDistanceErrorsWriter.h>
#include <sopnet/evaluation/VariationOfInformation.h>
#include <sopnet/evaluation/RandIndex.h>
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

util::ProgramOption optionSaveErrors(
		util::_long_name        = "saveErrors",
		util::_description_text = "Create an image stack for every split and merge error. Be careful, this can result in a lot of data.");

util::ProgramOption optionHeadless(
		util::_long_name        = "headless",
		util::_description_text = "Don't show the gui.");

util::ProgramOption optionVoi(
		util::_long_name        = "voi",
		util::_description_text = "Compute the variation of information as well.");

util::ProgramOption optionRand(
		util::_long_name        = "rand",
		util::_description_text = "Compute the Rand index as well.");

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

		// setup comparison measures

		pipeline::Process<VariationOfInformation> voi;
		pipeline::Process<RandIndex>              rand;

		// connect

		editDistance->setInput("ground truth", groundTruthReader->getOutput());
		editDistance->setInput("reconstruction", reconstructionReader->getOutput());

		if (optionVoi) {

			voi->setInput("stack 1", groundTruthReader->getOutput());
			voi->setInput("stack 2", reconstructionReader->getOutput());
		}

		if (optionRand) {

			rand->setInput("stack 1", groundTruthReader->getOutput());
			rand->setInput("stack 2", reconstructionReader->getOutput());
		}

		if (!optionHeadless) {

			// start GUI

			pipeline::Process<ImageStackView> gtView;
			pipeline::Process<ImageStackView> recView;
			pipeline::Process<ImageStackView> corRecView;
			pipeline::Process<ImageStackView> splitsView;
			pipeline::Process<ImageStackView> mergesView;
			pipeline::Process<ImageStackView> fpView;
			pipeline::Process<ImageStackView> fnView;
			pipeline::Process<gui::ZoomView>  zoomView;
			pipeline::Process<gui::Window>    window("edit distance");

			gtView->setInput(groundTruthReader->getOutput());
			recView->setInput(reconstructionReader->getOutput());
			corRecView->setInput(editDistance->getOutput("corrected reconstruction"));
			splitsView->setInput(editDistance->getOutput("splits"));
			mergesView->setInput(editDistance->getOutput("merges"));
			fpView->setInput(editDistance->getOutput("false positives"));
			fnView->setInput(editDistance->getOutput("false negatives"));

			pipeline::Process<gui::NamedView> gtNamedView("ground truth");
			pipeline::Process<gui::NamedView> recNamedView("reconstruction");
			pipeline::Process<gui::NamedView> corRecNamedView("corrected");
			pipeline::Process<gui::NamedView> splitsNamedView("splits");
			pipeline::Process<gui::NamedView> mergesNamedView("merges");
			pipeline::Process<gui::NamedView> fpNamedView("false positives");
			pipeline::Process<gui::NamedView> fnNamedView("false negatives");

			gtNamedView->setInput(gtView->getOutput());
			recNamedView->setInput(recView->getOutput());
			corRecNamedView->setInput(corRecView->getOutput());
			splitsNamedView->setInput(splitsView->getOutput());
			mergesNamedView->setInput(mergesView->getOutput());
			fpNamedView->setInput(fpView->getOutput());
			fnNamedView->setInput(fnView->getOutput());

			pipeline::Process<gui::ContainerView<gui::HorizontalPlacing> > container;
			container->setSpacing(10);
			container->setAlign(gui::HorizontalPlacing::Bottom);
			container->addInput(gtNamedView->getOutput());
			container->addInput(recNamedView->getOutput());
			container->addInput(corRecNamedView->getOutput());
			container->addInput(splitsNamedView->getOutput());
			container->addInput(mergesNamedView->getOutput());
			container->addInput(fpNamedView->getOutput());
			container->addInput(fnNamedView->getOutput());

			if (optionVoi) {

				pipeline::Process<gui::NamedView> voiNamedView("VOI");
				pipeline::Process<gui::NumberView<double> > voiView;

				voiView->setInput(voi->getOutput());
				voiNamedView->setInput(voiView->getOutput());
				container->addInput(voiNamedView->getOutput());
			}

			if (optionRand) {

				pipeline::Process<gui::NamedView> randNamedView("RAND");
				pipeline::Process<gui::NumberView<double> > randView;

				randView->setInput(rand->getOutput());
				randNamedView->setInput(randView->getOutput());
				container->addInput(randNamedView->getOutput());
			}

			zoomView->setInput(container->getOutput());
			window->setInput(zoomView->getOutput());

			window->processEvents();
		}

		// save results

		pipeline::Process<ImageStackDirectoryWriter> correctedWriter("corrected");
		pipeline::Process<ImageStackDirectoryWriter> splitsWriter("splits");
		pipeline::Process<ImageStackDirectoryWriter> mergesWriter("merges");
		pipeline::Process<ImageStackDirectoryWriter> fpWriter("false_positives");
		pipeline::Process<ImageStackDirectoryWriter> fnWriter("false_negatives");

		correctedWriter->setInput(editDistance->getOutput("corrected reconstruction"));
		splitsWriter->setInput(editDistance->getOutput("splits"));
		mergesWriter->setInput(editDistance->getOutput("merges"));
		fpWriter->setInput(editDistance->getOutput("false positives"));
		fnWriter->setInput(editDistance->getOutput("false negatives"));

		correctedWriter->write();
		splitsWriter->write();
		mergesWriter->write();
		fpWriter->write();
		fnWriter->write();

		if (optionSaveErrors) {

			pipeline::Process<TolerantEditDistanceErrorsWriter> errorsWriter;
			errorsWriter->setInput("ground truth", groundTruthReader->getOutput());
			errorsWriter->setInput("reconstruction", reconstructionReader->getOutput());
			errorsWriter->setInput("ted errors", editDistance->getOutput("errors"));

			errorsWriter->write("errors");
		}

	} catch (Exception& e) {

		handleException(e, std::cerr);
	}
}


