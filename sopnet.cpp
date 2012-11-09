/**
 * sopnet main file. Initializes all objects, views, and visualizers.
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
#include <inference/io/RandomForestHdf5Writer.h>
#include <imageprocessing/ImageExtractor.h>
#include <imageprocessing/SubStackSelector.h>
#include <imageprocessing/gui/ImageStackView.h>
#include <imageprocessing/io/ImageStackHdf5Reader.h>
#include <imageprocessing/io/ImageStackDirectoryReader.h>
#include <sopnet/Sopnet.h>
#include <sopnet/gui/SegmentsView.h>
#include <sopnet/gui/SegmentsStackView.h>
#include <sopnet/gui/SopnetDialog.h>
#include <sopnet/io/NeuronsImageWriter.h>
#include <sopnet/neurons/NeuronExtractor.h>
#include <util/hdf5.h>
#include <util/ProgramOptions.h>

using std::cout;
using std::endl;
using namespace gui;
using namespace logger;

util::ProgramOption optionProjectName(
		_long_name        = "project",
		_short_name       = "p",
		_description_text = "The HDF5 project file.");

util::ProgramOption optionSlicesFromStacks(
		_module           = "sopnet",
		_long_name        = "slicesFromStacks",
		_description_text = "Indicate that the 'slices' directory contains black/white image stacks (directories) of "
		                    "slice segmentation hypotheses.");

util::ProgramOption optionTraining(
		_long_name        = "train",
		_short_name       = "t",
		_description_text = "Train the segment random forest classifier.");

util::ProgramOption optionFirstSection(
		_module           = "sopnet",
		_long_name        = "firstSection",
		_description_text = "The number of the first section to process.",
		_default_value    = 0);

util::ProgramOption optionLastSection(
		_module           = "sopnet",
		_long_name        = "lastSection",
		_description_text = "The number of the last section to process. If set to -1, all sections after <firstSection> will be used.",
		_default_value    = -1);

util::ProgramOption optionShowAllSegments(
		_module           = "sopnet",
		_long_name        = "showAllSegments",
		_description_text = "Show all segment hypotheses.");

util::ProgramOption optionShowResult(
		_module           = "sopnet",
		_long_name        = "showResult",
		_description_text = "Show the result.");

util::ProgramOption optionShowResult3d(
		_module           = "sopnet",
		_long_name        = "showResult3d",
		_description_text = "Show a 3D view of the result.");

util::ProgramOption optionShowGroundTruth(
		_module           = "sopnet",
		_long_name        = "showGroundTruth",
		_description_text = "Show a 3D view of the ground-truth.");

util::ProgramOption optionShowGoldStandard(
		_module           = "sopnet",
		_long_name        = "showGoldStandard",
		_description_text = "Show a 3D view of the gold-standard.");

util::ProgramOption optionShowNegativeSamples(
		_module           = "sopnet",
		_long_name        = "showNegativeSamples",
		_description_text = "Show a 3D view of all negative training samples.");

util::ProgramOption optionSaveResultDirectory(
		_module           = "sopnet",
		_long_name        = "saveResultDirectory",
		_description_text = "The name of the directory to save the resulting id map to.");

util::ProgramOption optionSaveResultBasename(
		_module           = "sopnet",
		_long_name        = "saveResultBasename",
		_description_text = "The basenames of the images files created in the result directory. The default is \"result_\".",
		_default_value    = "result_");

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

		// create two rows of views
		boost::shared_ptr<ContainerView<VerticalPlacing> >   mainContainer       = boost::make_shared<ContainerView<VerticalPlacing> >();
		boost::shared_ptr<ContainerView<HorizontalPlacing> > imageStackContainer = boost::make_shared<ContainerView<HorizontalPlacing> >();
		boost::shared_ptr<ContainerView<HorizontalPlacing> > segmentsContainer   = boost::make_shared<ContainerView<HorizontalPlacing> >();

		// create sopnet dialog
		boost::shared_ptr<SopnetDialog> sopnetDialog = boost::make_shared<SopnetDialog>();

		// connect them to the window via the zoom view
		mainContainer->addInput(imageStackContainer->getOutput());
		mainContainer->addInput(sopnetDialog->getOutput("painter"));
		mainContainer->addInput(segmentsContainer->getOutput());
		zoomView->setInput(mainContainer->getOutput());

		// create basic views
		boost::shared_ptr<ImageStackView> rawSectionsView = boost::make_shared<ImageStackView>();
		boost::shared_ptr<ImageStackView> membranesView   = boost::make_shared<ImageStackView>();
		boost::shared_ptr<ImageStackView> slicesView      = boost::make_shared<ImageStackView>();
		boost::shared_ptr<ImageStackView> groundTruthView = boost::make_shared<ImageStackView>();

		// fill first row
		imageStackContainer->addInput(rawSectionsView->getOutput());
		imageStackContainer->addInput(membranesView->getOutput());
		imageStackContainer->addInput(slicesView->getOutput());
		imageStackContainer->addInput(groundTruthView->getOutput());

		// create section readers
		boost::shared_ptr<pipeline::ProcessNode> rawSectionsReader;
		boost::shared_ptr<pipeline::ProcessNode> membranesReader;
		boost::shared_ptr<pipeline::ProcessNode> slicesReader;
		boost::shared_ptr<pipeline::ProcessNode> groundTruthReader;

		boost::shared_ptr<pipeline::Wrap<std::vector<std::string> > > sliceStackDirectories = boost::make_shared<pipeline::Wrap<std::vector<std::string> > >();

		// create image stack readers
		if (!optionProjectName) {

			// if no project filename was given, try to read from default
			// directory
			rawSectionsReader = boost::make_shared<ImageStackDirectoryReader>("./raw/");
			membranesReader   = boost::make_shared<ImageStackDirectoryReader>("./membranes/");
			slicesReader      = boost::make_shared<ImageStackDirectoryReader>("./slices/");
			groundTruthReader = boost::make_shared<ImageStackDirectoryReader>("./groundtruth/");

			// list all directories under ./slices for the image stack option
			boost::filesystem::path dir("./slices/");

			if (!boost::filesystem::exists(dir))
				BOOST_THROW_EXCEPTION(IOError() << error_message(dir.string() + " does not exist"));

			if (!boost::filesystem::is_directory(dir))
				BOOST_THROW_EXCEPTION(IOError() << error_message(dir.string() + " is not a directory"));

			// get a sorted list of the directory contents
			std::vector<boost::filesystem::path> sorted;
			std::copy(
					boost::filesystem::directory_iterator(dir),
					boost::filesystem::directory_iterator(),
					back_inserter(sorted));
			std::sort(sorted.begin(), sorted.end());

			LOG_DEBUG(out) << "slice stack directory contains " << sorted.size() << " entries" << std::endl;

			// for every image directory in the given directory
			foreach (boost::filesystem::path directory, sorted)
				sliceStackDirectories->get().push_back(directory.string());

		} else {

			// get the project filename
			std::string projectFilename = optionProjectName;

			// try to read from project hdf5 file
			rawSectionsReader = boost::make_shared<ImageStackHdf5Reader>(projectFilename, "vncstack", "raw");
			membranesReader   = boost::make_shared<ImageStackHdf5Reader>(projectFilename, "vncstack", "membranes");
			slicesReader      = boost::make_shared<ImageStackHdf5Reader>(projectFilename, "vncstack", "slices");
			groundTruthReader = boost::make_shared<ImageStackHdf5Reader>(projectFilename, "vncstack", "groundtruth");
		}

		// select a substack, if options are set
		if (optionFirstSection || optionLastSection) {

			int firstSection = optionFirstSection;
			int lastSection  = optionLastSection;

			// create section selectors
			boost::shared_ptr<SubStackSelector> rawSelector         = boost::make_shared<SubStackSelector>(firstSection, lastSection);
			boost::shared_ptr<SubStackSelector> membranesSelector   = boost::make_shared<SubStackSelector>(firstSection, lastSection);
			boost::shared_ptr<SubStackSelector> groundTruthSelector = boost::make_shared<SubStackSelector>(firstSection, lastSection);

			// set their inputs to the outputs of the section readers
			rawSelector->setInput(rawSectionsReader->getOutput());
			membranesSelector->setInput(membranesReader->getOutput());
			groundTruthSelector->setInput(groundTruthReader->getOutput());

			// sneakily pretend the selectors are the readers
			rawSectionsReader = rawSelector;
			membranesReader   = membranesSelector;
			groundTruthReader = groundTruthSelector;

			// special case: select a subset of the slice hypotheses
			if (optionSlicesFromStacks) {

				if (firstSection >= sliceStackDirectories->get().size())
					BOOST_THROW_EXCEPTION(IOError() << error_message("not enough slice sections given for desired substack range"));

				if (lastSection >= sliceStackDirectories->get().size())
					BOOST_THROW_EXCEPTION(IOError() << error_message("not enough slice sections given for desired substack range"));

				boost::shared_ptr<pipeline::Wrap<std::vector<std::string> > > tmp = boost::make_shared<pipeline::Wrap<std::vector<std::string> > >();
				std::copy(
						sliceStackDirectories->get().begin() + firstSection,
						sliceStackDirectories->get().begin() + lastSection + 1,
						back_inserter(tmp->get()));
				sliceStackDirectories = tmp;

			} else {

				// do the same as with the other image stacks
				boost::shared_ptr<SubStackSelector> slicesSelector = boost::make_shared<SubStackSelector>(firstSection, lastSection);
				slicesSelector->setInput(slicesReader->getOutput());
				slicesReader = slicesSelector;
			}
		}

		// set input for image stack views
		rawSectionsView->setInput(rawSectionsReader->getOutput());
		membranesView->setInput(membranesReader->getOutput());
		slicesView->setInput(slicesReader->getOutput());
		groundTruthView->setInput(groundTruthReader->getOutput());

		// create sopnet pipeline
		boost::shared_ptr<Sopnet> sopnet = boost::make_shared<Sopnet>("projects dir not yet implemented");

		// set input to sopnet pipeline
		sopnet->setInput("raw sections", rawSectionsReader->getOutput());
		sopnet->setInput("membranes", membranesReader->getOutput());
		if (optionSlicesFromStacks)
			sopnet->setInput("slice stack directories", sliceStackDirectories);
		else
			sopnet->setInput("slices", slicesReader->getOutput());
		sopnet->setInput("ground truth", groundTruthReader->getOutput());
		sopnet->setInput("segmentation cost parameters", sopnetDialog->getOutput("segmentation cost parameters"));
		sopnet->setInput("prior cost parameters", sopnetDialog->getOutput("prior cost parameters"));
		sopnet->setInput("force explanation", sopnetDialog->getOutput("force explanation"));

		if (optionShowAllSegments) {

			boost::shared_ptr<ContainerView<OverlayPlacing> >  overlay      = boost::make_shared<ContainerView<OverlayPlacing> >();
			boost::shared_ptr<ImageStackView>                  sectionsView = boost::make_shared<ImageStackView>(3);
			boost::shared_ptr<SegmentsStackView>               resultView   = boost::make_shared<SegmentsStackView>(true);
			boost::shared_ptr<NamedView>                       namedView    = boost::make_shared<NamedView>("All Segments:");

			resultView->setInput(sopnet->getOutput("segments"));
			sectionsView->setInput(rawSectionsReader->getOutput());
			overlay->addInput(sectionsView->getOutput());
			overlay->addInput(resultView->getOutput());
			namedView->setInput(overlay->getOutput());

			segmentsContainer->addInput(namedView->getOutput());
		}

		if (optionShowResult) {

			boost::shared_ptr<ContainerView<OverlayPlacing> > overlay      = boost::make_shared<ContainerView<OverlayPlacing> >();
			boost::shared_ptr<ImageStackView>                 sectionsView = boost::make_shared<ImageStackView>();
			boost::shared_ptr<SegmentsStackView>              resultView   = boost::make_shared<SegmentsStackView>();
			boost::shared_ptr<NamedView>                      namedView    = boost::make_shared<NamedView>("Result:");

			resultView->setInput(sopnet->getOutput("solution"));
			sectionsView->setInput(rawSectionsReader->getOutput());
			overlay->addInput(sectionsView->getOutput());
			overlay->addInput(resultView->getOutput());
			namedView->setInput(overlay->getOutput());

			segmentsContainer->addInput(namedView->getOutput());
		}

		if (optionShowResult3d) {

			boost::shared_ptr<SegmentsView> resultView   = boost::make_shared<SegmentsView>();
			boost::shared_ptr<RotateView>   rotateView   = boost::make_shared<RotateView>();
			boost::shared_ptr<NamedView>    namedView    = boost::make_shared<NamedView>("Result:");

			resultView->setInput(sopnet->getOutput("solution"));
			rotateView->setInput(resultView->getOutput());
			namedView->setInput(rotateView->getOutput());

			segmentsContainer->addInput(namedView->getOutput());
		}

		if (optionShowGroundTruth) {

			boost::shared_ptr<SegmentsView> groundTruthView = boost::make_shared<SegmentsView>();
			boost::shared_ptr<RotateView>   gtRotateView    = boost::make_shared<RotateView>();
			boost::shared_ptr<NamedView>    namedView       = boost::make_shared<NamedView>("Ground-truth:");

			groundTruthView->setInput(sopnet->getOutput("ground truth segments"));
			groundTruthView->setInput("raw sections", rawSectionsReader->getOutput());
			gtRotateView->setInput(groundTruthView->getOutput());
			namedView->setInput(gtRotateView->getOutput());

			segmentsContainer->addInput(namedView->getOutput());
		}

		if (optionShowGoldStandard) {

			boost::shared_ptr<SegmentsView> goldstandardView = boost::make_shared<SegmentsView>();
			boost::shared_ptr<RotateView>   gsRotateView     = boost::make_shared<RotateView>();
			boost::shared_ptr<NamedView>    namedView        = boost::make_shared<NamedView>("Gold Standard:");

			goldstandardView->setInput(sopnet->getOutput("gold standard"));
			goldstandardView->setInput("raw sections", rawSectionsReader->getOutput());
			gsRotateView->setInput(goldstandardView->getOutput());
			namedView->setInput(gsRotateView->getOutput());

			segmentsContainer->addInput(namedView->getOutput());
		}

		if (optionShowNegativeSamples) {

			boost::shared_ptr<SegmentsView> negativeView = boost::make_shared<SegmentsView>();
			boost::shared_ptr<RotateView>   neRotateView = boost::make_shared<RotateView>();
			boost::shared_ptr<NamedView>    namedView    = boost::make_shared<NamedView>("Negative Samples:");

			negativeView->setInput(sopnet->getOutput("negative samples"));
			negativeView->setInput("raw sections", rawSectionsReader->getOutput());
			neRotateView->setInput(negativeView->getOutput());
			namedView->setInput(neRotateView->getOutput());

			segmentsContainer->addInput(namedView->getOutput());
		}

		if (optionTraining) {

			boost::shared_ptr<RandomForestHdf5Writer> rfWriter = boost::make_shared<RandomForestHdf5Writer>("./segment_rf.hdf");

			rfWriter->setInput("random forest", sopnet->getOutput("random forest"));
			rfWriter->write();

			LOG_USER(out) << "[main] training finished!" << std::endl;
		}

		while (!window->closed()) {

			window->processEvents();
			usleep(1000);
		}

		if (optionSaveResultDirectory) {

			LOG_USER(out) << "[main] writing solution to directory " << optionSaveResultDirectory.as<std::string>() << std::endl;

			boost::shared_ptr<NeuronExtractor>    neuronExtractor = boost::make_shared<NeuronExtractor>();
			boost::shared_ptr<NeuronsImageWriter> resultWriter    = boost::make_shared<NeuronsImageWriter>(optionSaveResultDirectory, optionSaveResultBasename);

			neuronExtractor->setInput("segments", sopnet->getOutput("solution"));
			resultWriter->setInput("neurons", neuronExtractor->getOutput());
			resultWriter->setInput("reference image stack", rawSectionsReader->getOutput());

			resultWriter->write();
		}

		LOG_USER(out) << "[main] exiting..." << std::endl;

	} catch (Exception& e) {

		handleException(e);
	}
}
