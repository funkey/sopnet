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

#include <imageprocessing/ComponentTreeDownSampler.h>
#include <imageprocessing/Mser.h>
#include <imageprocessing/io/ImageReader.h>
#include <imageprocessing/io/ComponentTreeHdf5Writer.h>
#include <imageprocessing/io/ImageStackHdf5Reader.h>
#include <imageprocessing/io/ImageStackDirectoryReader.h>
#include <imageprocessing/ImageExtractor.h>

#include <util/hdf5.h>
#include <util/ProgramOptions.h>

using std::cout;
using std::endl;
using namespace logger;
using namespace util;

util::ProgramOption optionProjectName(
		_long_name        = "project",
		_short_name       = "p",
		_description_text = "The HDF5 project file.");
        
util::ProgramOption optionHDF5(
		_long_name        = "hdf5",
		_short_name       = "o",
		_description_text = "The HDF5 file to write to.");
        
util::ProgramOption optionDownsample(
		_long_name        = "downsample",
		_short_name       = "d",
		_description_text = "Whether to downsample the component tree before writing. Downsampling removes all single children of the component tree.");

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
		boost::shared_ptr<pipeline::ProcessNode> membranesReader;

		{
            
            // create image stack readers
            if (!optionProjectName) {
                
                // if no project filename was given, try to read from default
                // directoryies
                membranesReader = boost::make_shared<ImageStackDirectoryReader>("./membranes/");

            } else {
                
                // get the project filename
                std::string projectFilename = optionProjectName;

                // try to read from project hdf5 file
                membranesReader = boost::make_shared<ImageStackHdf5Reader>(projectFilename, "vncstack", "membranes");
            }

            // LOG_ERROR(out) << "stack size" << membranesReader->getOutput() << std::endl;

			// get hdf5 file name
			std::string hdf5name = optionHDF5;
			if (hdf5name == "") {

				LOG_ERROR(out) << "no hdf5 output file given" << std::endl;

				exit(-1);
			}

			// open file
			H5::H5File hdf5file(hdf5name, H5F_ACC_TRUNC);

			// create a group for connected components
			H5::Group componentsGroup = hdf5file.createGroup("/connected_components");

			// create image reader
			// boost::shared_ptr<ImageReader> reader = boost::make_shared<ImageReader>(imagename);

            
            // create image extractor
            boost::shared_ptr<ImageExtractor> imageExtractor = boost::make_shared<ImageExtractor>();
            imageExtractor->setInput( membranesReader->getOutput() );
            
            // TODO: how to find out the max number of sections?
            for (int section = 0; section < imageExtractor->getNumImages(); section++) {
                
                LOG_USER(out) << "[loop] section..." << section << std::endl;

                H5::Group componentGroupSection = hdf5file.createGroup("/connected_components/" + boost::lexical_cast<std::string>(section));
                
                // create hdf5 writer
                boost::shared_ptr<ComponentTreeHdf5Writer> hdf5Writer = boost::make_shared<ComponentTreeHdf5Writer>(componentGroupSection);

                // create Mser
                boost::shared_ptr<Mser> mser = boost::make_shared<Mser>();

                // create down sampler
                boost::shared_ptr<ComponentTreeDownSampler> downsampler = boost::make_shared<ComponentTreeDownSampler>();

                // connect image reader to mser
                //mser->setInput("image", membranesReader->getOutput());
                mser->setInput("image", imageExtractor->getOutput(section));

                // set default parameters
                /*boost::shared_ptr<MserParameters> defaultParameters = boost::make_shared<MserParameters>();
                mser->setInput("parameters", defaultParameters);
                */
                boost::shared_ptr<MserParameters> mserParameters = boost::make_shared<MserParameters>();
                mserParameters->delta        = 1;
                mserParameters->minArea      = 10; // this is to avoid this tiny annotation that mess up the result
                mserParameters->maxArea      = 10000000;
                mserParameters->maxVariation = 100;
                mserParameters->minDiversity = 0;
                mserParameters->darkToBright = false;
                mserParameters->brightToDark = true;
                mserParameters->sameIntensityComponents = false; // only extract connected components of same intensity
                mser->setInput("parameters", mserParameters);

                if (optionDownsample) {
                    
                    LOG_USER(out) << "[loop] downsample..." << std::endl;

                    // connect mser to component tree downsampler
                    downsampler->setInput(mser->getOutput());

                    // connect downsampler output to hdf5 writer
                    hdf5Writer->setInput(downsampler->getOutput());

                } else {

                    // connect mser output to hdf5 writer
                    hdf5Writer->setInput(mser->getOutput());
                }

                hdf5Writer->write();
                
            } // end loop
            
		}

		LOG_USER(out) << "[main] exiting..." << std::endl;

	} catch (Exception& e) {

		std::cerr << "[main] caught exception: " << typeName(e) << std::endl;

		if (boost::get_error_info<error_message>(e))

			std::cerr << "[main] message: "
			          << *boost::get_error_info<error_message>(e)
			          << std::endl;

		std::cerr << "[main] details: " << std::endl
		          << boost::diagnostic_information(e)
		          << std::endl;
	}
}
