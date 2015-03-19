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

#include <exceptions.h>
#include <util/CommandLineParser.h>

#include <imageprocessing/ComponentTreeDownSampler.h>
#include <imageprocessing/Mser.h>
#include <imageprocessing/io/ImageReader.h>
#include <imageprocessing/io/ComponentTreeHdf5Writer.h>

using std::cout;
using std::endl;
using namespace logger;

util::CommandLineOption argImageFilename(
		"", "--image", "-i",
		"The image file.");

util::CommandLineOption argHdf5Filename(
		"", "--hdf5", "-h",
		"The HDF5 file to write to.");

util::CommandLineOption argDownsample(
		"", "--downsample", "-d",
		"Whether to downsample the component tree before writing. Downsampling removes all single children of the component tree.");

int main(int argc, char** argv) {

	try {

		/********
		 * INIT *
		 ********/

		// init command line parser
		util::CommandLineParser::init(argc, argv);

		// init logger
		LogManager::init();

		/*********
		 * SETUP *
		 *********/

		{
			// get image name
			std::string imagename = util::CommandLineParser::getOptionValue(argImageFilename);
			if (imagename == "") {

				LOG_ERROR(out) << "no image file given" << std::endl;

				util::CommandLineParser::printUsage();
				exit(-1);
			}

			// get hdf5 file name
			std::string hdf5name = util::CommandLineParser::getOptionValue(argHdf5Filename);
			if (hdf5name == "") {

				LOG_ERROR(out) << "no hdf5 file given" << std::endl;

				util::CommandLineParser::printUsage();
				exit(-1);
			}

			// open file
			H5::H5File hdf5file(hdf5name, H5F_ACC_TRUNC);

			// create a group for connected components
			H5::Group componentsGroup = hdf5file.createGroup("/connected_components");

			// create image reader
			boost::shared_ptr<ImageReader> reader = boost::make_shared<ImageReader>(imagename);

			// create ComponentTreeExtractor
			boost::shared_ptr<ComponentTreeExtractor> cte = boost::make_shared<ComponentTreeExtractor>();

			// create down sampler
			boost::shared_ptr<ComponentTreeDownSampler> downsampler = boost::make_shared<ComponentTreeDownSampler>();

			// create hdf5 writer
			boost::shared_ptr<ComponentTreeHdf5Writer> hdf5Writer = boost::make_shared<ComponentTreeHdf5Writer>(componentsGroup);

			// connect image reader to cte
			cte->setInput("image", reader->getOutput());

			// set default parameters
			boost::shared_ptr<MserParameters> defaultParameters = boost::make_shared<MserParameters>();
			cte->setInput("parameters", defaultParameters);

			if (util::CommandLineParser::isOptionSet(argDownsample)) {

				// connect cte to component tree downsampler
				downsampler->setInput(cte->getOutput());

				// connect downsampler output to hdf5 writer
				hdf5Writer->setInput(downsampler->getOutput());

			} else {

				// connect cte output to hdf5 writer
				hdf5Writer->setInput(cte->getOutput());
			}

			hdf5Writer->write();
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
