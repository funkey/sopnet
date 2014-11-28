#include <boost/timer/timer.hpp>
#include "IdMapCreator.h"

static logger::LogChannel idMapCreatorLog("idMapCreatorLog", "[IdMapCreator] ");

util::ProgramOption optionDrawSkeletons(
		util::_long_name        = "drawSkeletons",
		util::_description_text = "Store all neuron id maps as skeletons instead of volumetric processes.");

IdMapCreator::IdMapCreator():
	_idMap(new ImageStack()) {

	_useReference = true;	

	_drawSkeletons = optionDrawSkeletons;

	registerInput(_reference, "reference");

	init();
}

IdMapCreator::IdMapCreator(unsigned int numSections, unsigned int width, unsigned int height) :
	_idMap(new ImageStack()) {

	_useReference = false;
	
	_numSections = numSections;
	_width = width;
	_height = height;

	init();

}

void
IdMapCreator::init() {

	registerInput(_neurons, "neurons");
	registerOutput(_idMap, "id map");

}

void
IdMapCreator::updateOutputs() {

	boost::timer::auto_cpu_timer timer("\tIdMapCreator::updateOutputs():\t\t%ws\n");

	if ( _useReference ) { 
		// get widht, height and size from reference image stack
		_numSections = _reference->size();
		_width  = _reference->width();
		_height = _reference->height();
	}

	// prepare output images

	std::vector<boost::shared_ptr<Image> > idImages;
	
	for (unsigned int i = 0; i < _numSections; i++)
		idImages.push_back(boost::make_shared<Image>(_width, _height, 0.0));

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

	for (unsigned int i = 0; i < _numSections; i++)
		_idMap->add(idImages[i]);

}

void
IdMapCreator::drawSlice(const Slice& slice, std::vector<boost::shared_ptr<Image> >& images, unsigned int id) {

	unsigned int section = slice.getSection();

	if (_drawSkeletons) {

		util::point<unsigned int> center = slice.getComponent()->getCenter();

		(*images[section])(center.x, center.y) = static_cast<float>(id);

	} else {

		foreach (const util::point<unsigned int>& pixel, slice.getComponent()->getPixels()){

			(*images[section])(pixel.x, pixel.y) = static_cast<float>(id);
		}
	}
}
