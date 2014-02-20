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
#include <inference/io/RandomForestHdf5Writer.h>
#include <imageprocessing/ImageExtractor.h>
#include <imageprocessing/SubStackSelector.h>
#include <imageprocessing/io/ImageStackHdf5Reader.h>
#include <imageprocessing/io/ImageStackDirectoryReader.h>
#include <pipeline/Value.h>
#include <sopnet/Sopnet.h>
#include <sopnet/evaluation/ResultEvaluator.h>
#include <sopnet/evaluation/VariationOfInformation.h>
#include <sopnet/gui/SopnetDialog.h>
#include <sopnet/io/IdMapCreator.h>
#include <sopnet/io/NeuronsImageWriter.h>
#include <sopnet/inference/GridSearch.h>
#include <sopnet/neurons/NeuronExtractor.h>
#include <sopnet/inference/ProblemGraphWriter.h>
#include <util/ProgramOptions.h>
#include <util/SignalHandler.h>


using std::cout;
using std::endl;
using namespace gui;
using namespace logger;

util::ProgramOption optionDataRawName(
		_long_name        = "projectRaw",
		_short_name       = "pr",
		_description_text = "The HDF5 raw data file.");

util::ProgramOption optionDataMembraneName(
		_long_name        = "projectMembrane",
		_short_name       = "pm",
		_description_text = "The HDF5 membranes data file.");

util::ProgramOption optionDataSlicesName(
		_long_name        = "projectSlices",
		_short_name       = "ps",
		_description_text = "The HDF5 slices data file.");

util::ProgramOption optionSlicesFromStacks(
		_module           = "sopnet",
		_long_name        = "slicesFromStacks",
		_description_text = "Indicate that the 'slices' directory contains black/white image stacks (directories) of "
		                    "slice segmentation hypotheses.");

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

util::ProgramOption optionOriginSection(
		_module           = "sopnet",
		_long_name        = "originSection",
		_description_text = "The number of the origin section.",
		_default_value    = 0);

util::ProgramOption optionTargetSection(
		_module           = "sopnet",
		_long_name        = "targetSection",
		_description_text = "The number of the origin section.",
		_default_value    = 1);

util::ProgramOption optionMinXROI(
		_long_name        = "minX",
		_description_text = "The minX coordinate of the ROI.",
		_default_value    = 0);

util::ProgramOption optionMaxXROI(
		_long_name        = "maxX",
		_description_text = "The maxX coordinate of the ROI.",
		_default_value    = 0);

util::ProgramOption optionMinYROI(
		_long_name        = "minY",
		_description_text = "The minY coordinate of the ROI.",
		_default_value    = 0);

util::ProgramOption optionMaxYROI(
		_long_name        = "maxY",
		_description_text = "The maxY coordinate of the ROI.",
		_default_value    = 0);


int main(int optionc, char** optionv) {

	/********
	 * INIT *
	 ********/

	// init command line parser
	util::ProgramOptions::init(optionc, optionv);

	// init logger
	LogManager::init();

	// init signal handler
	util::SignalHandler::init();

	LOG_USER(out) << "[main] starting..." << std::endl;

	/*********
	 * SETUP *
	 *********/

	// create section readers
	boost::shared_ptr<pipeline::ProcessNode> rawSectionsReader;
	boost::shared_ptr<pipeline::ProcessNode> membranesReader;
	boost::shared_ptr<pipeline::ProcessNode> slicesReader;

	boost::shared_ptr<pipeline::Wrap<std::vector<std::string> > > sliceStackDirectories = boost::make_shared<pipeline::Wrap<std::vector<std::string> > >();

	int firstSection = optionFirstSection;;
	int lastSection = optionLastSection;

	int originSection = optionOriginSection;
	int targetSection  = optionTargetSection;

	// create image stack readers
	if (!optionDataRawName) {

		// if no project filename was given, try to read from default
		// directory
		rawSectionsReader = boost::make_shared<ImageStackDirectoryReader>("./raw/");
		membranesReader   = boost::make_shared<ImageStackDirectoryReader>("./membranes/");
		slicesReader      = boost::make_shared<ImageStackDirectoryReader>("./slices/");
		

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
				sliceStackDirectories->get().push_back(directory.string());

		// select a substack, if options are set
		if (optionFirstSection || optionLastSection) {

			// create section selectors
			boost::shared_ptr<SubStackSelector> rawSelector         = boost::make_shared<SubStackSelector>(firstSection, lastSection);
			boost::shared_ptr<SubStackSelector> membranesSelector   = boost::make_shared<SubStackSelector>(firstSection, lastSection);
			
			// set their inputs to the outputs of the section readers
			rawSelector->setInput(rawSectionsReader->getOutput());
			membranesSelector->setInput(membranesReader->getOutput());
			
			// sneakily pretend the selectors are the readers
			rawSectionsReader = rawSelector;
			membranesReader   = membranesSelector;

			// special case: select a subset of the slice hypotheses
			if (optionSlicesFromStacks) {

				if (firstSection >= (int)sliceStackDirectories->get().size())
					BOOST_THROW_EXCEPTION(IOError() << error_message("not enough slice sections given for desired substack range"));

				if (lastSection >= (int)sliceStackDirectories->get().size())
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

	} else {

#ifdef HAVE_HDF5

		// get the project filename
		std::string dataRawFilename = optionDataRawName;
		std::string dataMembraneFilename = optionDataMembraneName;
		std::string dataSlicesFilename = optionDataSlicesName;

		// try to read from project hdf5 file
		rawSectionsReader = boost::make_shared<ImageStackHdf5Reader>(
				dataRawFilename,
				"0",
				"data", 
				originSection,
				targetSection,
				optionMinXROI,
				optionMaxXROI,
				optionMinYROI,
				optionMaxYROI);

		membranesReader = boost::make_shared<ImageStackHdf5Reader>(
				dataMembraneFilename,
				"0",
				"data",
				originSection,
				targetSection,
				optionMinXROI,
				optionMaxXROI,
				optionMinYROI,
				optionMaxYROI);

		if (optionDataSlicesName) {

			slicesReader = boost::make_shared<ImageStackHdf5Reader>(
					dataSlicesFilename,
					"0",
					"data",
					originSection,
					targetSection,
					optionMinXROI,
					optionMaxXROI,
					optionMinYROI,
					optionMaxYROI);

		} else {
			slicesReader = boost::make_shared<ImageStackHdf5Reader>(
					dataMembraneFilename,
					"0",
					"data",
					originSection,
					targetSection,
					optionMinXROI,
					optionMaxXROI,
					optionMinYROI,
					optionMaxYROI);
		}

#endif // HAVE_HDF5

	}

	// create problem writer
	boost::shared_ptr<ProblemGraphWriter> problemWriter = boost::make_shared<ProblemGraphWriter>();

	// create sopnet pipeline
	boost::shared_ptr<Sopnet> sopnet = boost::make_shared<Sopnet>("projects dir not yet implemented", problemWriter);

	// set input to sopnet pipeline
	sopnet->setInput("raw sections", rawSectionsReader->getOutput());
	sopnet->setInput("membranes", membranesReader->getOutput());
	if (optionSlicesFromStacks)
		sopnet->setInput("slice stack directories", sliceStackDirectories);
	else
		sopnet->setInput("slices", slicesReader->getOutput());
	

	boost::shared_ptr<PriorCostFunctionParameters> priors = boost::make_shared<PriorCostFunctionParameters>();
	priors->priorEnd = PriorCostFunctionParameters::optionPriorEnds.as<double>();
	priors->priorContinuation = PriorCostFunctionParameters::optionPriorContinuations.as<double>();
	priors->priorBranch = PriorCostFunctionParameters::optionPriorBranches.as<double>();

	sopnet->setInput("segmentation cost parameters", boost::make_shared<SegmentationCostFunctionParameters>());
	sopnet->setInput("prior cost parameters", priors);
	sopnet->setInput("force explanation", pipeline::Value<bool>(true));

	problemWriter->write("./dump/slices.txt", "./dump/segments.txt", "./dump/constraints.txt", "./dump/slices/", originSection, targetSection);

	LOG_USER(out) << "[main] exiting..." << std::endl;

}
