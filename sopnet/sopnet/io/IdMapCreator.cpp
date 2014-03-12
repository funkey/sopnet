#include "IdMapCreator.h"

IdMapCreator::IdMapCreator() :
	_idMap(new ImageStack()) {

	registerInput(_neurons, "neurons");
	registerInput(_reference, "reference");

	registerOutput(_idMap, "id map");
}

void
IdMapCreator::updateOutputs() {

	// get widht and height of output images from reference image stack

	unsigned int numSections = _reference->size();
	unsigned int width  = _reference->width();
	unsigned int height = _reference->height();

	// prepare output images

	std::vector<boost::shared_ptr<Image> > idImages;
	
	for (unsigned int i = 0; i < numSections; i++)
		idImages.push_back(boost::make_shared<Image>(width, height, 0.0));

	// draw each neuron to output images

	unsigned int id = 1;

	foreach (boost::shared_ptr<SegmentTree> neuron, *_neurons) {

		foreach (boost::shared_ptr<EndSegment> end, neuron->getEnds()) {

			drawSlice(*end->getSlice(), idImages, id);
		}

		foreach (boost::shared_ptr<ContinuationSegment> continuation, neuron->getContinuations()) {

			if (continuation->getDirection() == Left)

				drawSlice(*continuation->getSourceSlice(), idImages, id);

			else

				drawSlice(*continuation->getTargetSlice(), idImages, id);
		}

		foreach (boost::shared_ptr<BranchSegment> branch, neuron->getBranches()) {

			if (branch->getDirection() == Left)

				drawSlice(*branch->getSourceSlice(), idImages, id);

			else {

				drawSlice(*branch->getTargetSlice1(), idImages, id);
				drawSlice(*branch->getTargetSlice2(), idImages, id);
			}
		}

		id++;
	}

	// store output images in image stack

	_idMap->clear();

	for (unsigned int i = 0; i < numSections; i++)
		_idMap->add(idImages[i]);
}

void
IdMapCreator::drawSlice(const Slice& slice, std::vector<boost::shared_ptr<Image> >& images, unsigned int id) {

	unsigned int section = slice.getSection();

	foreach (const util::point<unsigned int>& pixel, slice.getComponent()->getPixels())
		(*images[section])(pixel.x, pixel.y) = static_cast<float>(id);
}
