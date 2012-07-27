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
#include <imageprocessing/io/SegmentsHdf5Writer.h>
#include <sopnet/Sopnet.h>
#include <sopnet/gui/SegmentsView.h>
#include <sopnet/gui/SegmentsStackView.h>
#include <sopnet/gui/SopnetDialog.h>
#include <sopnet/inference/SegmentationCostFunctionParameters.h>
#include <sopnet/inference/PriorCostFunctionParameters.h>
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

util::ProgramOption optionDumpSegments(
		_module           = "sopnet",
		_long_name        = "dumpSegments",
		_description_text = "Dump the segments to the HDF5 file.");

util::ProgramOption optionHDF5(
		_long_name        = "hdf5",
		_short_name       = "o",
		_description_text = "The HDF5 file to write to.");

// add more parameters such as option distance threshold

void handleException(boost::exception& e) {

    std::cerr << "[main] caught exception: " << typeName(e) << std::endl;

    if (boost::get_error_info<error_message>(e))

        std::cerr << "[main] message: "
                  << *boost::get_error_info<error_message>(e)
                  << std::endl;

    std::cerr << "[main] details: " << std::endl
              << boost::diagnostic_information(e)
              << std::endl;

	exit(-1);
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

		// create section readers
		boost::shared_ptr<pipeline::ProcessNode> rawSectionsReader;
		boost::shared_ptr<pipeline::ProcessNode> membranesReader;
		boost::shared_ptr<pipeline::ProcessNode> slicesReader;
		boost::shared_ptr<pipeline::ProcessNode> groundTruthReader;

		// create image stack readers
		if (!optionProjectName) {

			// if no project filename was given, try to read from default
			// directoryies
			rawSectionsReader = boost::make_shared<ImageStackDirectoryReader>("./raw/");
			membranesReader   = boost::make_shared<ImageStackDirectoryReader>("./membranes/");
			slicesReader      = boost::make_shared<ImageStackDirectoryReader>("./slices/");
			groundTruthReader = boost::make_shared<ImageStackDirectoryReader>("./groundtruth/");

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
			boost::shared_ptr<SubStackSelector> slicesSelector      = boost::make_shared<SubStackSelector>(firstSection, lastSection);
			boost::shared_ptr<SubStackSelector> groundTruthSelector = boost::make_shared<SubStackSelector>(firstSection, lastSection);

			// set their inputs to the outputs of the section readers
			rawSelector->setInput(rawSectionsReader->getOutput());
			membranesSelector->setInput(membranesReader->getOutput());
			slicesSelector->setInput(slicesReader->getOutput());
			groundTruthSelector->setInput(groundTruthReader->getOutput());

			// sneakily pretend the selectors are the readers
			rawSectionsReader = rawSelector;
			membranesReader   = membranesSelector;
			slicesReader      = slicesSelector;
			groundTruthReader = groundTruthSelector;
		}

		// create sopnet pipeline
		boost::shared_ptr<Sopnet> sopnet = boost::make_shared<Sopnet>("projects dir not yet implemented");

		// set input to sopnet pipeline
		sopnet->setInput("raw sections", rawSectionsReader->getOutput());
		sopnet->setInput("membranes", slicesReader->getOutput());
		sopnet->setInput("ground truth", groundTruthReader->getOutput());
        
        
        boost::shared_ptr<SegmentationCostFunctionParameters> segcostparms = boost::make_shared<SegmentationCostFunctionParameters>();
        boost::shared_ptr<PriorCostFunctionParameters> priorcostparams = boost::make_shared<PriorCostFunctionParameters>();
		sopnet->setInput("segmentation cost parameters", segcostparms);
		sopnet->setInput("prior cost parameters", priorcostparams);

		if (optionTraining) {

			boost::shared_ptr<RandomForestHdf5Writer> rfWriter = boost::make_shared<RandomForestHdf5Writer>("./segment_rf.hdf");

			rfWriter->setInput("random forest", sopnet->getOutput("random forest"));
			rfWriter->write();

			LOG_USER(out) << "[main] training finished!" << std::endl;
		}

        // write segments to hdf
        if (optionDumpSegments) {
            // ...
            
			// get hdf5 file name
			std::string hdf5name = optionHDF5;
			if (hdf5name == "") {

				LOG_ERROR(out) << "no hdf5 output file given" << std::endl;

				exit(-1);
			}
            
			// open file
			H5::H5File hdf5file(hdf5name, H5F_ACC_TRUNC);

			// create a group for connected components
			H5::Group componentsGroup = hdf5file.createGroup("/segments");

            // TODO: for each pair of sections
            H5::Group segmentGroupSection = hdf5file.createGroup("/segments/" + boost::lexical_cast<std::string>(0));
            
			boost::shared_ptr<SegmentsHdf5Writer> segmentsWriter = boost::make_shared<SegmentsHdf5Writer>(segmentGroupSection);

			segmentsWriter->setInput("segments", sopnet->getOutput("segments"));
            segmentsWriter->setInput("cost function", sopnet->getOutput("cost function"));
			segmentsWriter->write();

            hdf5file.close();

			LOG_USER(out) << "[main] dumping segments finished!" << std::endl;
            
        }

		LOG_USER(out) << "[main] exiting..." << std::endl;

	} catch (Exception& e) {

		handleException(e);
	}
}
