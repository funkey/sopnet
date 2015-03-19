#ifndef SOPNET_GUI_NEURON_VOLUME_ADAPTOR_H__
#define SOPNET_GUI_NEURON_VOLUME_ADAPTOR_H__

#include <sopnet/segments/SegmentTree.h>

class NeuronVolumeAdaptor {

public:

	typedef Image::value_type value_type;

	NeuronVolumeAdaptor(const SegmentTree& neuron) :
		_neuron(neuron) {

		if (neuron.getBoundingBox().volume() == 0)
			return;

		unsigned int numSections = neuron.getBoundingBox().depth()/neuron.getResolutionZ() + 1;

		_slices.resize(numSections);

		// collect all slices
		foreach (boost::shared_ptr<Segment> segment, neuron.getSegments()) {

			unsigned int lowerSection = (segment->getBoundingBox().getMinZ() - neuron.getBoundingBox().getMinZ())/neuron.getResolutionZ();
			unsigned int sourceSection, targetSection;

			if (segment->getDirection() == Right) {

				sourceSection = lowerSection;
				targetSection = lowerSection + 1;

			} else {

				sourceSection = lowerSection + 1;
				targetSection = lowerSection;
			}

			foreach (boost::shared_ptr<Slice> slice, segment->getSourceSlices())
				_slices[sourceSection].push_back(slice);
			foreach (boost::shared_ptr<Slice> slice, segment->getTargetSlices())
				_slices[targetSection].push_back(slice);
		}
	}

	const BoundingBox& getBoundingBox() const { return _neuron.getBoundingBox(); }

	float operator()(float x, float y, float z) const {

		unsigned int dx, dy, dz, section;

		_neuron.getDiscreteCoordinates(x, y, z, dx, dy, section);

		if (section >= _slices.size())
			return 0.0;

		foreach (boost::shared_ptr<Slice> slice, _slices[section]) {

			const BoundingBox& sliceBoundingBox = slice->getBoundingBox();

			if (!sliceBoundingBox.contains(x, y, z))
				continue;

			slice->getDiscreteCoordinates(x, y, z, dx, dy, dz);

			if (slice->getComponent()->getBitmap()(dx, dy))
				return 1.0;
			else
				return 0.0;
		}

		return 0.0;
	}

private:

	const SegmentTree& _neuron;

	// the slices of the neuron, sorted by section
	std::vector<std::vector<boost::shared_ptr<Slice> > > _slices;
};

#endif // SOPNET_GUI_NEURONS_VOLUME_ADAPTOR_H__

