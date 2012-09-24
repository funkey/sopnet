#ifndef IMAGEPROCESSING_IO_IMAGE_STACK_HDF5_READER_H__
#define IMAGEPROCESSING_IO_IMAGE_STACK_HDF5_READER_H__

#include <util/hdf5.h>

#include <pipeline/all.h>
#include <imageprocessing/ImageStack.h>

class ImageStackHdf5Reader : public pipeline::SimpleProcessNode<> {

public:

	ImageStackHdf5Reader(
		const std::string& filename,
		const std::string& groupname,
		const std::string& datasetname);

private:

	void updateOutputs();

	void readImages();

	// the name of the hdf5 file
	std::string _filename;

	// the name of the group that contains the dataset
	std::string _groupname;

	// the name of the dataset that contains the image stack
	std::string _datasetname;

	pipeline::Output<ImageStack> _stack;
};

#endif // IMAGEPROCESSING_IO_IMAGE_STACK_HDF5_READER_H__

