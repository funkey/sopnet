#ifndef SOPNET_IO_ID_MAP_CREATOR_H__
#define SOPNET_IO_ID_MAP_CREATOR_H__

#include <pipeline/all.h>

#include <imageprocessing/ImageStack.h>
#include <sopnet/segments/SegmentTrees.h>

/**
 * Creates an image stack from a set of neurons, such that same intensity values 
 * correspond to pixels of the same neuron.
 */
class IdMapCreator : public pipeline::SimpleProcessNode<> {

public:

	IdMapCreator();

private:

	void updateOutputs();

	void drawSlice(const Slice& slice, std::vector<boost::shared_ptr<Image> >& images, unsigned int id);

	pipeline::Input<SegmentTrees> _neurons;
	pipeline::Input<ImageStack>   _reference;
	pipeline::Output<ImageStack>  _idMap;
};

#endif // SOPNET_IO_ID_MAP_CREATOR_H__

