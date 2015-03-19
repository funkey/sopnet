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
#include <gui/MeshView.h>
#include <gui/NamedView.h>
#include <gui/RotateView.h>
#include <gui/Window.h>
#include <gui/ZoomView.h>
#include <inference/io/RandomForestHdf5Writer.h>
#include <imageprocessing/SubStackSelector.h>
#include <imageprocessing/gui/ImageStackView.h>
#include <imageprocessing/io/ImageStackHdf5Reader.h>
#include <imageprocessing/io/ImageStackDirectoryReader.h>
#include <sopnet/Sopnet.h>
#include <sopnet/evaluation/ErrorReport.h>
#include <sopnet/gui/ErrorsView.h>
#include <sopnet/gui/FeaturesView.h>
#include <sopnet/gui/NeuronsMeshExtractor.h>
#include <sopnet/gui/NeuronsView.h>
#include <sopnet/gui/NeuronsStackView.h>
#include <sopnet/gui/SegmentsView.h>
#include <sopnet/gui/SegmentsStackView.h>
#include <sopnet/gui/SopnetDialog.h>
#include <sopnet/io/IdMapCreator.h>
#include <sopnet/io/NeuronsImageWriter.h>
#include <sopnet/inference/GridSearch.h>
#include <sopnet/neurons/NeuronExtractor.h>
#include <util/ProgramOptions.h>
#include <util/SignalHandler.h>

#include <config.h>

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

util::ProgramOption optionWriteStructuredProblem(
		_long_name        = "writeStructuredProblem",
		_description_text = "Dump the gold standard, all features and constraints for structured learning.");

util::ProgramOption optionWriteMinimalImpactTED(
		_long_name	  = "writeMinimalImpactTED",
		_description_text = "Dump coefficients for minimal impact TED for structured learning.");

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

util::ProgramOption optionShowSegmentFeatures(
		_module           = "sopnet",
		_long_name        = "showSegmentFeatures",
		_description_text = "Show the features of the currently selected segment.");

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

util::ProgramOption optionShowNeurons(
		_module           = "sopnet",
		_long_name        = "showNeurons",
		_description_text = "Show a 3D view for each found neuron.");

util::ProgramOption optionShowErrors(
		_module           = "sopnet",
		_long_name        = "showErrors",
		_description_text = "Show the errors of the result.");

util::ProgramOption optionSaveResultDirectory(
		_module           = "sopnet",
		_long_name        = "saveResultDirectory",
		_description_text = "The name of the directory to save the resulting id map to.");

util::ProgramOption optionSaveResultBasename(
		_module           = "sopnet",
		_long_name        = "saveResultBasename",
		_description_text = "The basenames of the images files created in the result directory. The default is \"result_\".",
		_default_value    = "result_");

util::ProgramOption optionGridSearch(
		_module           = "sopnet.inference",
		_long_name        = "gridSearch",
		_description_text = "Preform a grid search.");

util::ProgramOption optionForceExplanation(
		util::_module           = "sopnet.inference",
		util::_long_name        = "forceExplanation",
		util::_description_text = "Force the solver to explain every segmentation hypotheses, i.e., that in every maximal "
		                          "set of conflicting slices exactly one gets chosen. This usually results in a denser "
		                          "reconstruction. This works only for slice hypotheses coming from a component tree, which "
		                          "means it cannot be combined with 'slicesFromStacks'.");

util::ProgramOption optionHeadless(
		_long_name        = "headless",
		_description_text = "Despite other options that suggest a GUI (like showErrors), do not start the GUI.");

void processEvents(boost::shared_ptr<gui::Window>& window) {

	LOG_USER(out) << " started as " << window->getCaption() << " at " << window.get() << std::endl;

	while (!window->closed()) {

		try {

			window->processEvents();

		} catch (boost::exception& e) {

			handleException(e, std::cerr);
		}

		usleep(1000);
	}

	LOG_USER(out) << "[window thread] releasing shared pointer to window" << std::endl;

	LOG_USER(out) << "[window thread] quitting" << std::endl;
}

int main(int optionc, char** optionv) {

	// let the windows be the last thing to be destructed
	boost::shared_ptr<gui::Window> resultWindow;
	boost::shared_ptr<gui::Window> controlWindow;

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

		int firstSection = std::max(0,optionFirstSection.as<int>());
		int lastSection  = optionLastSection;
		SliceHashConfiguration::sectionOffset = firstSection;

		/*********
		 * SETUP *
		 *********/

		// GUI elements
		boost::shared_ptr<gui::ZoomView> resultZoomView;
		boost::shared_ptr<gui::ZoomView> controlZoomView;
		boost::shared_ptr<ContainerView<VerticalPlacing> >   resultContainer;
		boost::shared_ptr<ContainerView<VerticalPlacing> >   controlContainer;
		boost::shared_ptr<ContainerView<HorizontalPlacing> > imageStackContainer;
		boost::shared_ptr<ContainerView<HorizontalPlacing> > segmentsContainer;
		boost::shared_ptr<SopnetDialog> sopnetDialog;

		if (!optionHeadless) {

			// create windows
			resultWindow  = boost::make_shared<gui::Window>("sopnet: results");
			controlWindow = boost::make_shared<gui::Window>("sopnet: controls");

			// create a zoom view for this window
			resultZoomView  = boost::make_shared<gui::ZoomView>(true);
			controlZoomView = boost::make_shared<gui::ZoomView>(true);

			resultWindow->setInput(resultZoomView->getOutput());
			controlWindow->setInput(controlZoomView->getOutput());

			// create two rows of views
			resultContainer     = boost::make_shared<ContainerView<VerticalPlacing> >("results");
			controlContainer    = boost::make_shared<ContainerView<VerticalPlacing> >("controls");
			imageStackContainer = boost::make_shared<ContainerView<HorizontalPlacing> >("image stacks");
			segmentsContainer   = boost::make_shared<ContainerView<HorizontalPlacing> >("segments");

			// create sopnet dialog
			sopnetDialog = boost::make_shared<SopnetDialog>(optionForceExplanation.as<bool>());

			// connect them to the window via the zoom view
			controlContainer->setAlign(VerticalPlacing::Left);
			controlContainer->addInput(imageStackContainer->getOutput());
			controlContainer->addInput(sopnetDialog->getOutput("painter"));
			controlZoomView->setInput(controlContainer->getOutput());
			resultContainer->addInput(segmentsContainer->getOutput());
			resultZoomView->setInput(resultContainer->getOutput());
		}

		// create section readers
		boost::shared_ptr<pipeline::ProcessNode> rawSectionsReader;
		boost::shared_ptr<pipeline::ProcessNode> membranesReader;
		boost::shared_ptr<pipeline::ProcessNode> slicesReader;
		boost::shared_ptr<pipeline::ProcessNode> mitochondriaReader;
		boost::shared_ptr<pipeline::ProcessNode> synapseReader;
		boost::shared_ptr<pipeline::ProcessNode> groundTruthReader;

		pipeline::Value<std::vector<std::string> > sliceStackDirectories;

		// create image stack readers
		if (!optionProjectName) {

			// if no project filename was given, try to read from default
			// directory
			rawSectionsReader = boost::make_shared<ImageStackDirectoryReader>("./raw/");
			membranesReader   = boost::make_shared<ImageStackDirectoryReader>("./membranes/");
			slicesReader      = boost::make_shared<ImageStackDirectoryReader>("./slices/");
			boost::filesystem::path mitoDir("./mitochondria/");
			if (boost::filesystem::is_directory(mitoDir)) {
				LOG_USER(out) << "found a mitochondria directory" << std::endl;
				mitochondriaReader = boost::make_shared<ImageStackDirectoryReader>(mitoDir.string());
			}
			boost::filesystem::path synDir("./synapses/");
			if (boost::filesystem::is_directory(synDir)) {
				LOG_USER(out) << "found a synapse directory" << std::endl;
				synapseReader = boost::make_shared<ImageStackDirectoryReader>(synDir.string());
			}
			boost::filesystem::path gtDir("./groundtruth/");
			if (boost::filesystem::is_directory(gtDir)) {
				LOG_USER(out) << "found a ground-truth directory" << std::endl;
				groundTruthReader = boost::make_shared<ImageStackDirectoryReader>(gtDir.string());
			}

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
				if (boost::filesystem::is_directory(directory))
					sliceStackDirectories->push_back(directory.string());

		} else {

#ifdef HAVE_HDF5

			// get the project filename
			std::string projectFilename = optionProjectName;

			// try to read from project hdf5 file
			rawSectionsReader = boost::make_shared<ImageStackHdf5Reader>(projectFilename, "0", "data", firstSection, lastSection, 0, 0, 256, 256);
			membranesReader   = boost::make_shared<ImageStackHdf5Reader>(projectFilename, "0", "data", firstSection, lastSection, 0, 0, 256, 256);
			slicesReader      = boost::make_shared<ImageStackHdf5Reader>(projectFilename, "0", "data", firstSection, lastSection, 0, 0, 256, 256);
			groundTruthReader = boost::make_shared<ImageStackHdf5Reader>(projectFilename, "0", "data", firstSection, lastSection, 0, 0, 256, 256);

#else

			LOG_ERROR(out) << "HDF5 not supported -- please recompile with HDF5 enabled" << std::endl;

#endif
		}

		// select a substack, if options are set
		if (firstSection != 0 || lastSection != -1) {

			// create section selectors
			boost::shared_ptr<SubStackSelector> rawSelector          = boost::make_shared<SubStackSelector>(firstSection, lastSection);
			boost::shared_ptr<SubStackSelector> membranesSelector    = boost::make_shared<SubStackSelector>(firstSection, lastSection);
			boost::shared_ptr<SubStackSelector> mitochondriaSelector = boost::make_shared<SubStackSelector>(firstSection, lastSection);
			boost::shared_ptr<SubStackSelector> synapseSelector      = boost::make_shared<SubStackSelector>(firstSection, lastSection);
			boost::shared_ptr<SubStackSelector> groundTruthSelector  = boost::make_shared<SubStackSelector>(firstSection, lastSection);

			// set their inputs to the outputs of the section readers
			rawSelector->setInput(rawSectionsReader->getOutput());
			membranesSelector->setInput(membranesReader->getOutput());
			if (mitochondriaReader)
				mitochondriaSelector->setInput(mitochondriaReader->getOutput());
			if (synapseReader)
				synapseSelector->setInput(synapseReader->getOutput());
			if (groundTruthReader)
				groundTruthSelector->setInput(groundTruthReader->getOutput());

			// sneakily pretend the selectors are the readers
			rawSectionsReader = rawSelector;
			membranesReader   = membranesSelector;
			if (mitochondriaReader)
				mitochondriaReader = mitochondriaSelector;
			if (synapseReader)
				synapseReader = synapseSelector;
			if (groundTruthReader)
				groundTruthReader = groundTruthSelector;

			// special case: select a subset of the slice hypotheses
			if (optionSlicesFromStacks) {

				if (firstSection >= (int)sliceStackDirectories->size())
					BOOST_THROW_EXCEPTION(IOError() << error_message("not enough slice sections given for desired substack range"));

				if (lastSection >= (int)sliceStackDirectories->size())
					BOOST_THROW_EXCEPTION(IOError() << error_message("not enough slice sections given for desired substack range"));

				pipeline::Value<std::vector<std::string> > tmp;
				std::copy(
						sliceStackDirectories->begin() + firstSection,
						sliceStackDirectories->begin() + lastSection + 1,
						back_inserter(*tmp));
				sliceStackDirectories = tmp;

			} else {

				// do the same as with the other image stacks
				boost::shared_ptr<SubStackSelector> slicesSelector = boost::make_shared<SubStackSelector>(firstSection, lastSection);
				slicesSelector->setInput(slicesReader->getOutput());
				slicesReader = slicesSelector;
			}
		}

		// create sopnet pipeline
		boost::shared_ptr<Sopnet> sopnet = boost::make_shared<Sopnet>("projects dir not yet implemented");

		// set input to sopnet pipeline
		sopnet->setInput("raw sections", rawSectionsReader->getOutput());
		sopnet->setInput("membranes", membranesReader->getOutput());
		sopnet->setInput("segmentation cost parameters", boost::make_shared<SegmentationCostFunctionParameters>());
		sopnet->setInput("prior cost parameters", boost::make_shared<PriorCostFunctionParameters>());
		pipeline::Value<bool> forceExplanation = optionForceExplanation.as<bool>();
		sopnet->setInput("force explanation", forceExplanation);
		if (optionSlicesFromStacks)
			sopnet->setInput("neuron slice stack directories", sliceStackDirectories);
		else
			sopnet->setInput("neuron slices", slicesReader->getOutput());
		if (mitochondriaReader)
			sopnet->setInput("mitochondria slices", mitochondriaReader->getOutput());
		if (synapseReader)
			sopnet->setInput("synapse slices", synapseReader->getOutput());
		if (groundTruthReader)
			sopnet->setInput("ground truth", groundTruthReader->getOutput());
		else if (optionTraining || optionWriteStructuredProblem) {

			LOG_ERROR(out) << "trainig requested, but no ground-truth found!" << std::endl;
			return -1;
		}

		// create result evaluator and variation of information if needed by 
		// anyone
		boost::shared_ptr<ErrorReport>            errorReport;

		if (optionShowErrors || optionGridSearch) {

			errorReport = boost::make_shared<ErrorReport>();

			errorReport->setInput("ground truth", groundTruthReader->getOutput());
			errorReport->setInput("ground truth segments", sopnet->getOutput("ground truth segments"));
			errorReport->setInput("gold standard segments", sopnet->getOutput("gold standard"));
			errorReport->setInput("reconstruction segments", sopnet->getOutput("solution"));
		}

		// create a neuron extractor
		boost::shared_ptr<NeuronExtractor> neuronExtractor = boost::make_shared<NeuronExtractor>();
		neuronExtractor->setInput("segments", sopnet->getOutput("solution"));

		// create a result id map creator if needed
		boost::shared_ptr<IdMapCreator> resultIdMapCreator;

		if (optionShowErrors || optionSaveResultDirectory || optionGridSearch) {

			resultIdMapCreator = boost::make_shared<IdMapCreator>();

			resultIdMapCreator->setInput("neurons", neuronExtractor->getOutput());
			resultIdMapCreator->setInput("reference", rawSectionsReader->getOutput());
		}

		if (!optionHeadless) {

			// create basic views
			boost::shared_ptr<ImageStackView> rawSectionsView = boost::make_shared<ImageStackView>();
			boost::shared_ptr<ImageStackView> membranesView   = boost::make_shared<ImageStackView>();
			boost::shared_ptr<ImageStackView> slicesView      = boost::make_shared<ImageStackView>();
			boost::shared_ptr<ImageStackView> groundTruthView = boost::make_shared<ImageStackView>();

			// fill first row
			imageStackContainer->addInput(rawSectionsView->getOutput());
			imageStackContainer->addInput(membranesView->getOutput());
			imageStackContainer->addInput(slicesView->getOutput());
			if (groundTruthReader)
				imageStackContainer->addInput(groundTruthView->getOutput());

			// set input for image stack views
			rawSectionsView->setInput(rawSectionsReader->getOutput());
			membranesView->setInput(membranesReader->getOutput());
			slicesView->setInput(slicesReader->getOutput());
			if (groundTruthReader)
				groundTruthView->setInput(groundTruthReader->getOutput());

			sopnet->setInput("segmentation cost parameters", sopnetDialog->getOutput("segmentation cost parameters"));
			sopnet->setInput("prior cost parameters", sopnetDialog->getOutput("prior cost parameters"));
			sopnet->setInput("force explanation", sopnetDialog->getOutput("force explanation"));

			if (optionShowAllSegments) {

				boost::shared_ptr<ContainerView<HorizontalPlacing> > container    = boost::make_shared<ContainerView<HorizontalPlacing> >("all segments");
				boost::shared_ptr<ContainerView<OverlayPlacing> >    overlay      = boost::make_shared<ContainerView<OverlayPlacing> >("stack view");
				boost::shared_ptr<ImageStackView>                    sectionsView = boost::make_shared<ImageStackView>(3, 5.0);
				boost::shared_ptr<SegmentsStackView>                 stackView    = boost::make_shared<SegmentsStackView>(5.0);
				boost::shared_ptr<SegmentsView>                      segmentsView = boost::make_shared<SegmentsView>("single segment");
				boost::shared_ptr<RotateView>                        rotateView   = boost::make_shared<RotateView>();
				boost::shared_ptr<NamedView>                         namedView    = boost::make_shared<NamedView>("All Segments:");

				stackView->setInput(sopnet->getOutput("segments"));
				sectionsView->setInput(rawSectionsReader->getOutput());
				overlay->addInput(stackView->getOutput("painter"));
				overlay->addInput(sectionsView->getOutput());

				segmentsView->setInput(stackView->getOutput("visible segments"));
				rotateView->setInput(segmentsView->getOutput());

				container->addInput(overlay->getOutput());

				if (optionShowSegmentFeatures) {

					boost::shared_ptr<FeaturesView> featuresView = boost::make_shared<FeaturesView>();
					featuresView->setInput("segments", stackView->getOutput("visible segments"));
					featuresView->setInput("features", sopnet->getOutput("all features"));
					featuresView->setInput("problem configuration", sopnet->getOutput("problem configuration"));
					featuresView->setInput("objective", sopnet->getOutput("objective"));

					if (optionShowGoldStandard)
						featuresView->setInput("ground truth score", sopnet->getOutput("gold standard objective"));

					container->addInput(featuresView->getOutput());
				}

				container->addInput(rotateView->getOutput());

				namedView->setInput(container->getOutput());

				controlContainer->addInput(namedView->getOutput());
			}

			boost::shared_ptr<NeuronsStackView> resultView;

			if (optionShowResult) {

				resultView = boost::make_shared<NeuronsStackView>();

				boost::shared_ptr<ContainerView<OverlayPlacing> > overlay      = boost::make_shared<ContainerView<OverlayPlacing> >("result");
				boost::shared_ptr<ImageStackView>                 sectionsView = boost::make_shared<ImageStackView>();
				boost::shared_ptr<NamedView>                      namedView    = boost::make_shared<NamedView>("Result:");

				resultView->setInput(neuronExtractor->getOutput("neurons"));
				sectionsView->setInput(rawSectionsReader->getOutput());
				overlay->addInput(resultView->getOutput());
				overlay->addInput(sectionsView->getOutput());
				namedView->setInput(overlay->getOutput());

				segmentsContainer->addInput(namedView->getOutput());
			}

			if (optionShowResult3d) {

				boost::shared_ptr<NeuronsMeshExtractor> meshExtractor = boost::make_shared<NeuronsMeshExtractor>();
				boost::shared_ptr<MeshView>             result3DView  = boost::make_shared<MeshView>();
				boost::shared_ptr<RotateView>           rotateView    = boost::make_shared<RotateView>();
				boost::shared_ptr<NamedView>            namedView     = boost::make_shared<NamedView>("Result:");

				meshExtractor->setInput(resultView->getOutput("selection"));
				result3DView->setInput(meshExtractor->getOutput());
				rotateView->setInput(result3DView->getOutput());
				namedView->setInput(rotateView->getOutput());

				segmentsContainer->addInput(namedView->getOutput());
			}

			if (optionShowGroundTruth) {

				boost::shared_ptr<NeuronExtractor>  gtNeuronExtractor = boost::make_shared<NeuronExtractor>();
				boost::shared_ptr<NeuronsStackView> groundTruthView   = boost::make_shared<NeuronsStackView>();
				boost::shared_ptr<NamedView>        namedView         = boost::make_shared<NamedView>("Ground-truth:");

				gtNeuronExtractor->setInput(sopnet->getOutput("ground truth segments"));
				groundTruthView->setInput(gtNeuronExtractor->getOutput());
				namedView->setInput(groundTruthView->getOutput());

				segmentsContainer->addInput(namedView->getOutput());
			}

			boost::shared_ptr<NeuronExtractor> gsNeuronExtractor;

			if (optionShowGoldStandard) {

				boost::shared_ptr<NeuronsStackView>               goldstandardView = boost::make_shared<NeuronsStackView>();
				boost::shared_ptr<ImageStackView>                 sectionsView     = boost::make_shared<ImageStackView>();
				boost::shared_ptr<ContainerView<OverlayPlacing> > overlay          = boost::make_shared<ContainerView<OverlayPlacing> >("result");
				boost::shared_ptr<NamedView>                      namedView        = boost::make_shared<NamedView>("Gold Standard:");

				gsNeuronExtractor = boost::make_shared<NeuronExtractor>();
				gsNeuronExtractor->setInput("segments", sopnet->getOutput("gold standard"));

				goldstandardView->setInput(gsNeuronExtractor->getOutput());
				sectionsView->setInput(rawSectionsReader->getOutput());
				overlay->addInput(goldstandardView->getOutput());
				overlay->addInput(sectionsView->getOutput());
				namedView->setInput(overlay->getOutput());

				segmentsContainer->addInput(namedView->getOutput());
			}

			if (optionShowNegativeSamples) {

				boost::shared_ptr<SegmentsView> negativeView = boost::make_shared<SegmentsView>("negative samples");
				boost::shared_ptr<RotateView>   neRotateView = boost::make_shared<RotateView>();
				boost::shared_ptr<NamedView>    namedView    = boost::make_shared<NamedView>("Negative Samples:");

				negativeView->setInput(sopnet->getOutput("negative samples"));
				negativeView->setInput("raw sections", rawSectionsReader->getOutput());
				neRotateView->setInput(negativeView->getOutput());
				namedView->setInput(neRotateView->getOutput());

				segmentsContainer->addInput(namedView->getOutput());
			}

			if (optionShowNeurons) {

				boost::shared_ptr<NeuronsView> neuronsView = boost::make_shared<NeuronsView>();
				boost::shared_ptr<NamedView>   namedView   = boost::make_shared<NamedView>("Found Neurons:");

				neuronsView->setInput(neuronExtractor->getOutput());
				if (optionShowErrors)
					neuronsView->setInput("aed errors", errorReport->getOutput("aed errors"));
				namedView->setInput(neuronsView->getOutput());

				controlContainer->addInput(namedView->getOutput());

				if (resultView)
					resultView->setInput("current neuron", neuronsView->getOutput("current neuron"));
			}

			if (optionShowErrors && groundTruthReader) {

				if (optionShowResult) {

					boost::shared_ptr<ErrorsView> errorsView = boost::make_shared<ErrorsView>();
					boost::shared_ptr<NamedView>  namedView  = boost::make_shared<NamedView>("Errors:");

					errorsView->setInput("error report", errorReport->getOutput("human readable error report"));
					namedView->setInput(errorsView->getOutput());

					resultContainer->addInput(namedView->getOutput());
				}

				if (optionShowGoldStandard) {

					// gold standard error
					pipeline::Process<NamedView>   goldStandardNamedView("Error bounds with current hypotheses:");
					pipeline::Process<ErrorReport> goldStandardErrorReport;
					pipeline::Process<ErrorsView>  goldStandardErrorsView;

					goldStandardErrorReport->setInput("ground truth", groundTruthReader->getOutput());
					goldStandardErrorReport->setInput("ground truth segments", sopnet->getOutput("ground truth segments"));
					goldStandardErrorReport->setInput("gold standard segments", sopnet->getOutput("gold standard"));
					goldStandardErrorReport->setInput("reconstruction segments", sopnet->getOutput("gold standard"));

					goldStandardErrorsView->setInput("error report", goldStandardErrorReport->getOutput("human readable error report"));
					goldStandardNamedView->setInput(goldStandardErrorsView->getOutput());

					resultContainer->addInput(goldStandardNamedView->getOutput());
				}
			}
		} // !optionHeadless

		if (optionHeadless && optionShowErrors && groundTruthReader && !optionGridSearch) {

			// Compute errors even if headless is on
			LOG_USER(out) << "Computing HEADLESS ERRORS:" << std::endl;

			pipeline::Value<std::string> reportHeader = errorReport->getOutput("error report header");
			pipeline::Value<std::string> reportLine = errorReport->getOutput("error report");

			LOG_USER(out) << *reportHeader << std::endl;
			LOG_USER(out) << *reportLine   << std::endl;
		}

		// Compute gold standard error in headless mode if show gold standard option is set
		if (optionHeadless && optionShowGoldStandard && groundTruthReader && !optionGridSearch) {

			boost::shared_ptr<ErrorReport> goldStandardErrorReport = boost::make_shared<ErrorReport>();

			goldStandardErrorReport->setInput("ground truth", groundTruthReader->getOutput());
			goldStandardErrorReport->setInput("ground truth segments", sopnet->getOutput("ground truth segments"));
			goldStandardErrorReport->setInput("gold standard segments", sopnet->getOutput("gold standard"));
			goldStandardErrorReport->setInput("reconstruction segments", sopnet->getOutput("gold standard"));

			LOG_USER(out) << "Computing GOLDSTANDARD HEADLESS ERRORS:" << std::endl;

			pipeline::Value<std::string> reportHeaderGoldStandard = goldStandardErrorReport->getOutput("error report header");
			pipeline::Value<std::string> reportLineGoldStandard = goldStandardErrorReport->getOutput("error report");

			LOG_USER(out) << *reportHeaderGoldStandard << std::endl;
			LOG_USER(out) << *reportLineGoldStandard   << std::endl;
		}

		if (optionTraining) {

			boost::shared_ptr<RandomForestHdf5Writer> rfWriter = boost::make_shared<RandomForestHdf5Writer>("./segment_rf.hdf");

			rfWriter->setInput("random forest", sopnet->getOutput("random forest"));
			rfWriter->write();

			LOG_USER(out) << "[main] training finished!" << std::endl;
		}

		if (optionWriteStructuredProblem) {

			sopnet->writeStructuredProblem("./labels.txt", "./features.txt", "./constraints.txt");

			LOG_USER(out) << "[main] files for structured learning written!" << std::endl;
		}

		if (optionWriteMinimalImpactTED) {
			
			sopnet->writeMinimalImpactTEDCoefficients("./minimalImpactTEDcoefficients.txt");

			LOG_USER(out) << "[main] file for minimal impact TED written!" << std::endl;
		}

		if (optionGridSearch) {

			LOG_USER(out) << "[main] performing grid search" << std::endl;

			boost::shared_ptr<GridSearch> gridSearch = boost::make_shared<GridSearch>();

			sopnet->setInput("prior cost parameters", gridSearch->getOutput("prior cost parameters"));
			sopnet->setInput("segmentation cost parameters", gridSearch->getOutput("segmentation cost parameters"));

			pipeline::Value<std::string> reportHeader = errorReport->getOutput("error report header");
			pipeline::Value<std::string> reportLine   = errorReport->getOutput("error report");

			// open file for grid search results
			std::ofstream gridSearchFile("grid_search.txt");
			gridSearchFile
					<< "# end\tcont\tbranch\tseg_w\tseg_p\tseg_f:\t"
					<< *reportHeader << std::endl;

			while (true) {

				std::string parameterString = gridSearch->currentParameters();

				LOG_USER(out) << "[main] performing grid search with " << parameterString << std::endl;

				gridSearchFile << parameterString << "\t" << *reportLine << std::endl;

				if (!gridSearch->next())
					break;
			}

			LOG_USER(out) << "[main] grid search done." << std::endl;
		}

		if (optionSaveResultDirectory) {

			LOG_USER(out) << "[main] writing solution to directory " << optionSaveResultDirectory.as<std::string>() << std::endl;

			boost::shared_ptr<NeuronsImageWriter> resultWriter =
					boost::make_shared<NeuronsImageWriter>(
							optionSaveResultDirectory,
							optionSaveResultBasename,
							optionFirstSection);

			resultWriter->setInput(resultIdMapCreator->getOutput("id map"));
			resultWriter->write();
		}

		// finally, show the windows
		if (!optionHeadless) {

			boost::thread controlThread(boost::bind(&processEvents, controlWindow));
			boost::thread resultThread(boost::bind(&processEvents, resultWindow));

			resultThread.join();
			controlThread.join();

			resultWindow->close();
			resultThread.join();
		}

		LOG_USER(out) << "[main] exiting..." << std::endl;

	} catch (Exception& e) {

		handleException(e, std::cerr);
	}
}
