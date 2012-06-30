#include "SegmentationCostFunction.h"
#include <imageprocessing/ConnectedComponent.h>
#include <sopnet/segments/EndSegment.h>
#include <sopnet/segments/ContinuationSegment.h>
#include <sopnet/segments/BranchSegment.h>
#include <sopnet/slices/Slice.h>

SegmentationCostFunction::SegmentationCostFunction() :
	_costFunction(boost::bind(&SegmentationCostFunction::costs, this, _1, _2, _3, _4)) {

	registerInput(_membranes, "membranes");
	registerInput(_parameters, "parameters");

	registerOutput(_costFunction, "cost function");
}

void
SegmentationCostFunction::updateOutputs() {

	// nothing to do here
}

void
SegmentationCostFunction::costs(
		const std::vector<boost::shared_ptr<EndSegment> >&          ends,
		const std::vector<boost::shared_ptr<ContinuationSegment> >& continuations,
		const std::vector<boost::shared_ptr<BranchSegment> >&       branches,
		std::vector<double>& segmentCosts) {

	segmentCosts.resize(ends.size() + continuations.size() + branches.size(), 0);

	unsigned int i = 0;

	foreach (boost::shared_ptr<EndSegment> end, ends) {

		segmentCosts[i] += _parameters->weight*costs(*end);

		i++;
	}

	foreach (boost::shared_ptr<ContinuationSegment> continuation, continuations) {

		segmentCosts[i] += _parameters->weight*costs(*continuation);

		i++;
	}

	foreach (boost::shared_ptr<BranchSegment> branch, branches) {

		segmentCosts[i] += _parameters->weight*costs(*branch);

		i++;
	}
}

double
SegmentationCostFunction::costs(const EndSegment& end) {

	return 0.5*costs(*end.getSlice());
}

double
SegmentationCostFunction::costs(const ContinuationSegment& continuation) {

	return 0.5*(costs(*continuation.getSourceSlice()) + costs(*continuation.getTargetSlice()));
}

double
SegmentationCostFunction::costs(const BranchSegment& branch) {

	return 0.5*(costs(*branch.getSourceSlice()) + costs(*branch.getTargetSlice1()) + costs(*branch.getTargetSlice2()));
}

double
SegmentationCostFunction::costs(const Slice& slice) {

	unsigned int section = slice.getSection();

	double costs = 0.0;

	// for each pixel in the slice
	foreach (const util::point<unsigned int>& pixel, slice.getComponent()->getPixels()) {

		double probMembrane = std::max(0.001, std::min(0.999, (*(*_membranes)[section])(pixel.x, pixel.y)/255.0));
		double probNeuron   = 1.0 - probMembrane;

		costs += - log(probMembrane) - log(probNeuron);
	}

	// for each boundary crack
	costs += getBoundaryLength(slice)*_parameters->weightPotts;

	return costs;
}

unsigned int
SegmentationCostFunction::getBoundaryLength(const Slice& slice) {

	boost::shared_ptr<ConnectedComponent> component = slice.getComponent();

	unsigned int width   = (unsigned int)(component->getBoundingBox().width()  + 1);
	unsigned int height  = (unsigned int)(component->getBoundingBox().height() + 1);
	unsigned int offsetX = (unsigned int)component->getBoundingBox().minX;
	unsigned int offsetY = (unsigned int)component->getBoundingBox().minY;

	std::vector<bool> pixels(width*height, false);

	foreach (const util::point<unsigned int>& p, component->getPixels()) {

		unsigned int x = p.x - offsetX;
		unsigned int y = p.y - offsetY;

		pixels[x + y*width] = true;
	}

	unsigned int boundaryLength = 0;

	foreach (const util::point<unsigned int>& p, component->getPixels()) {

		// for neighborhood testing with offset
		unsigned int x = p.x - offsetX;
		unsigned int y = p.y - offsetY;

		// left
		if (x == 0 || !pixels[(x - 1) + y*width])
			boundaryLength++;

		// right
		if (x == width - 1 || !pixels[(x + 1) + y*width])
			boundaryLength++;

		// top
		if (y == 0 || !pixels[x + (y - 1)*width])
			boundaryLength++;

		// bottom
		if (y == height - 1 || !pixels[x + (y + 1)*width])
			boundaryLength++;
	}

	return boundaryLength;
}
