#include "SegmentationCostFunction.h"
#include <imageprocessing/ConnectedComponent.h>
#include <sopnet/segments/EndSegment.h>
#include <sopnet/segments/ContinuationSegment.h>
#include <sopnet/segments/BranchSegment.h>
#include <sopnet/slices/Slice.h>
#include <util/ProgramOptions.h>

logger::LogChannel segmentationcostfunctionlog("segmentationcostfunctionlog", "[SegmentationCostFunction] ");

util::ProgramOption optionInvertMembraneMaps(
		util::_module           = "sopnet",
		util::_long_name        = "invertMembraneMaps",
		util::_description_text = "Invert the meaning of the membrane map. The default "
		                          "(not inverting) is: bright pixel = hight membrane probability.");

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

	if (_segmentationCosts.size() != segmentCosts.size() ||
	    _parameters->priorForeground != _prevParameters.priorForeground) {

		LOG_DEBUG(segmentationcostfunctionlog)
				<< "updating segmentation costs (number of segments: "
				<< segmentCosts.size() << ", number of cached values "
				<< _segmentationCosts.size() << ")" << std::endl;

		_segmentationCosts.clear();
		_segmentationCosts.reserve(ends.size() + continuations.size() + branches.size());

		computeSegmentationCosts(ends, continuations, branches);

		LOG_DEBUG(segmentationcostfunctionlog)
				<< "computed " << _segmentationCosts.size() << " segmentation cost values" << std::endl;
	}

	if (_boundaryLengths.size() != segmentCosts.size()) {

		LOG_DEBUG(segmentationcostfunctionlog) << "updating boundary lengths..." << std::endl;

		_boundaryLengths.clear();
		_boundaryLengths.reserve(ends.size() + continuations.size() + branches.size());

		computeBoundaryLengths(ends, continuations, branches);
	}

	_prevParameters = *_parameters;

	unsigned int i = 0;

	foreach (boost::shared_ptr<EndSegment> end, ends) {

		segmentCosts[i] += _parameters->weight*(_segmentationCosts[i] + _parameters->weightPotts*_boundaryLengths[i]);

		i++;
	}

	foreach (boost::shared_ptr<ContinuationSegment> continuation, continuations) {

		segmentCosts[i] += _parameters->weight*(_segmentationCosts[i] + _parameters->weightPotts*_boundaryLengths[i]);

		i++;
	}

	foreach (boost::shared_ptr<BranchSegment> branch, branches) {

		segmentCosts[i] += _parameters->weight*(_segmentationCosts[i] + _parameters->weightPotts*_boundaryLengths[i]);

		i++;
	}
}

void
SegmentationCostFunction::computeSegmentationCosts(
		const std::vector<boost::shared_ptr<EndSegment> >&          ends,
		const std::vector<boost::shared_ptr<ContinuationSegment> >& continuations,
		const std::vector<boost::shared_ptr<BranchSegment> >&       branches) {

	foreach (boost::shared_ptr<EndSegment> end, ends)
		computeSegmentationCost(*end);

	foreach (boost::shared_ptr<ContinuationSegment> continuation, continuations)
		computeSegmentationCost(*continuation);

	foreach (boost::shared_ptr<BranchSegment> branch, branches)
		computeSegmentationCost(*branch);
}

void
SegmentationCostFunction::computeBoundaryLengths(
		const std::vector<boost::shared_ptr<EndSegment> >&          ends,
		const std::vector<boost::shared_ptr<ContinuationSegment> >& continuations,
		const std::vector<boost::shared_ptr<BranchSegment> >&       branches) {

	foreach (boost::shared_ptr<EndSegment> end, ends)
		computeBoundaryLength(*end);

	foreach (boost::shared_ptr<ContinuationSegment> continuation, continuations)
		computeBoundaryLength(*continuation);

	foreach (boost::shared_ptr<BranchSegment> branch, branches)
		computeBoundaryLength(*branch);
}

void
SegmentationCostFunction::computeSegmentationCost(const EndSegment& end) {

	_segmentationCosts.push_back(computeSegmentationCost(*end.getSlice()));
}

void
SegmentationCostFunction::computeSegmentationCost(const ContinuationSegment& continuation) {

	_segmentationCosts.push_back(
			computeSegmentationCost(*continuation.getSourceSlice()) +
			computeSegmentationCost(*continuation.getTargetSlice()));
}

void
SegmentationCostFunction::computeSegmentationCost(const BranchSegment& branch) {

	_segmentationCosts.push_back(
			computeSegmentationCost(*branch.getSourceSlice()) +
			computeSegmentationCost(*branch.getTargetSlice1()) +
			computeSegmentationCost(*branch.getTargetSlice2()));
}

void
SegmentationCostFunction::computeBoundaryLength(const EndSegment& end) {

	_boundaryLengths.push_back(computeBoundaryLength(*end.getSlice()));
}

void
SegmentationCostFunction::computeBoundaryLength(const ContinuationSegment& continuation) {

	_boundaryLengths.push_back(
			computeBoundaryLength(*continuation.getSourceSlice()) +
			computeBoundaryLength(*continuation.getTargetSlice()));
}

void
SegmentationCostFunction::computeBoundaryLength(const BranchSegment& branch) {

	_boundaryLengths.push_back(
			computeBoundaryLength(*branch.getSourceSlice()) +
			computeBoundaryLength(*branch.getTargetSlice1()) +
			computeBoundaryLength(*branch.getTargetSlice2()));
}

double
SegmentationCostFunction::computeSegmentationCost(const Slice& slice) {

	if (_sliceSegmentationCosts.count(slice.getId()))
		return _sliceSegmentationCosts[slice.getId()];

	unsigned int section = slice.getSection();

	double costs = 0.0;

	// for each pixel in the slice
	foreach (const util::point<unsigned int>& pixel, slice.getComponent()->getPixels()) {

		// get the membrane data probability p(x|y=membrane)
		double probMembrane = (*(*_membranes)[section])(pixel.x, pixel.y)/255.0;

		if (optionInvertMembraneMaps)
			probMembrane = 1.0 - probMembrane;

		// get the neuron data probability p(x|y=neuron)
		double probNeuron = 1.0 - probMembrane;

		// multiply both with the respective prior p(y)
		probMembrane *= (1.0 - _parameters->priorForeground);
		probNeuron   *= _parameters->priorForeground;

		// normalize both probabilities, so that we get p(y|x)
		probMembrane /= probMembrane + probNeuron;
		probNeuron   /= probMembrane + probNeuron;

		// ensure numerical stability
		probMembrane = std::max(0.0001, std::min(0.9999, probMembrane));
		probNeuron   = std::max(0.0001, std::min(0.9999, probNeuron));

		// compute the corresponding costs
		double costsMembrane = -log(probMembrane);
		double costsNeuron   = -log(probNeuron);

		// costs for accepting the segmentation is the cost difference between
		// segmenting the region as background and segmenting the region as
		// foreground
		costs += costsNeuron - costsMembrane;
	}

	_sliceSegmentationCosts[slice.getId()] = costs;

	return costs;
}

unsigned int
SegmentationCostFunction::computeBoundaryLength(const Slice& slice) {

	if (_sliceBoundaryLengths.count(slice.getId()))
		return _sliceBoundaryLengths[slice.getId()];

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

	_sliceBoundaryLengths[slice.getId()] = boundaryLength;

	return boundaryLength;
}
