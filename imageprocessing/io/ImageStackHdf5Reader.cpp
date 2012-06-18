#include <util/Logger.h>
#include "ImageStackHdf5Reader.h"

logger::LogChannel imagestackhdf5readerlog("imagestackhdf5readerlog", "[ImageStackHdf5Reader] ");

ImageStackHdf5Reader::ImageStackHdf5Reader(
		const std::string& filename,
		const std::string& groupname,
		const std::string& datasetname) :
	_filename(filename),
	_groupname(groupname),
	_datasetname(datasetname) {

	registerOutput(_stack, "stack");
}

void
ImageStackHdf5Reader::updateOutputs() {

	readImages();
}

void
ImageStackHdf5Reader::readImages() {

	// open file
	H5::H5File file(_filename, H5F_ACC_RDONLY);

	// open group
	H5::Group group = file.openGroup(_groupname);

	// clear the previous stack
	_stack->clear();

	// read dataset
	std::vector<unsigned char> data = hdf5::read<unsigned char>(group, _datasetname);

	// get the dimensions
	std::vector<size_t> dims = hdf5::dimensions(group, _datasetname);

	unsigned int width    = dims[0];
	unsigned int height   = dims[1];
	unsigned int sections = dims[2];

	// create images
	for (int i = 0; i < sections; i++) {

		// create a shared float vector that stores the actual data
		boost::shared_ptr<std::vector<float> > imageData = boost::make_shared<std::vector<float> >(width*height);

		for (int j = 0; j < width*height; j++)
			(*imageData)[j] = (float)data[j*sections + i]/255.0;

		boost::shared_ptr<Image> section = boost::make_shared<Image>(width, height, imageData);

		_stack->add(section);
	}

	LOG_DEBUG(imagestackhdf5readerlog) << "read " << sections << " sections" << std::endl;
}
