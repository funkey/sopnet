#ifndef SOPNET_IO_NEURONS_IMAGE_WRITER_H__
#define SOPNET_IO_NEURONS_IMAGE_WRITER_H__

#include <string>

#include <pipeline/all.h>
#include <imageprocessing/ImageStack.h>
#include <sopnet/neurons/Neurons.h>

/**
 * Writes a set of neurons to a sequence of png images. The intensity of the 
 * images corresponds to the id of the neurons.
 */
class NeuronsImageWriter : public pipeline::SimpleProcessNode<> {

	typedef vigra::MultiArray<2, unsigned int> id_image;

public:

	NeuronsImageWriter(std::string directory, std::string basename);

	void write();

private:

	void updateOutputs() {}

	void drawSlice(const Slice& slice, std::vector<id_image>& idImages, unsigned int id);

	pipeline::Input<Neurons>    _neurons;
	pipeline::Input<ImageStack> _reference;

	std::string _directory;
	std::string _basename;
};

#endif // SOPNET_IO_NEURONS_IMAGE_WRITER_H__

