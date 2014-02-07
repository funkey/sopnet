#ifndef SOPNET_IO_NEURONS_IMAGE_WRITER_H__
#define SOPNET_IO_NEURONS_IMAGE_WRITER_H__

#include <string>

#include <pipeline/all.h>
#include <imageprocessing/ImageStack.h>
#include <sopnet/segments/SegmentTrees.h>

/**
 * Writes a set of neurons to a sequence of png images. The intensity of the 
 * images corresponds to the id of the neurons.
 */
class NeuronsImageWriter : public pipeline::SimpleProcessNode<> {

public:

	/**
	 * Create a neuron image writer for the given directory and basename, 
	 * starting counting the produced image files at firstSection.
	 */
	NeuronsImageWriter(
			std::string directory,
			std::string basename, 
			unsigned int firstSection = 0);

	void write();

private:

	void updateOutputs() {}

	pipeline::Input<ImageStack> _idMap;
	pipeline::Input<double>     _annotation;

	std::string  _directory;
	std::string  _basename;
	unsigned int _firstSection;
};

#endif // SOPNET_IO_NEURONS_IMAGE_WRITER_H__

