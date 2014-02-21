#include <util/foreach.h>
#include <imageprocessing/ConnectedComponent.h>
#include <pipeline/Value.h>
#include <imageprocessing/MserParameters.h>
#include <sopnet/slices/SliceExtractor.h>
#include "SliceEditor.h"

logger::LogChannel sliceeditorlog("sliceeditorlog", "[SliceEditor] ");

SliceEditor::SliceEditor(const std::vector<boost::shared_ptr<Slice> >& initialSlices, unsigned int section, const util::rect<int>& region) :
	_initialSlices(initialSlices),
	_section(section),
	_region(region),
	_sliceImage(boost::make_shared<Image>()) {

	// allocate image
	_sliceImage->reshape(region.width(), region.height());

	// draw initial slices
	_sliceImage->init(0.0);
	foreach (boost::shared_ptr<Slice> slice, _initialSlices)
		drawSlice(slice);
}

boost::shared_ptr<Image>
SliceEditor::getSliceImage() {

	return _sliceImage;
}

void
SliceEditor::draw(const util::point<double>& position, double radius, bool foreground) {

	util::point<int> p = position - _region.upperLeft();

	for (int dx = -radius; dx <= radius; dx++)
		for (int dy = -radius; dy <= radius; dy++)
			if (dx*dx + dy*dy <= radius*radius)
				if (p.x + dx >= 0 && p.x + dx < _sliceImage->width()
				&&  p.y + dy >= 0 && p.y + dy < _sliceImage->height())
					(*_sliceImage)(p.x + dx, p.y + dy) = (foreground ? 1.0 : 0.0);
}

SliceEdits
SliceEditor::finish() {

	SliceEdits edits;

	// create mser parameters suitable to extract connected
	// components
	pipeline::Value<MserParameters> mserParameters;
	mserParameters->delta        = 1;
	mserParameters->minArea      = 0;
	mserParameters->maxArea      = 10000000;
	mserParameters->maxVariation = 100;
	mserParameters->minDiversity = 0;
	mserParameters->darkToBright = false;
	mserParameters->brightToDark = true;
	mserParameters->sameIntensityComponents = false;

	LOG_DEBUG(sliceeditorlog) << "extracting slices from current slice image" << std::endl;

	// create a SliceExtractor
	pipeline::Process<SliceExtractor<unsigned short> > sliceExtractor(_section, false /* don't downsample */);

	// give it the section it has to process and our mser parameters
	sliceExtractor->setInput("membrane", _sliceImage);
	sliceExtractor->setInput("mser parameters", mserParameters);

	// get the slices in the current section
	pipeline::Value<Slices> slices = sliceExtractor->getOutput("slices");

	LOG_ALL(sliceeditorlog) << "found " << slices->size() << " slices" << std::endl;

	// TODO:
	// • create overlap map from initial to new slices
	// • turn overlap map into slice edits

	return edits;
}

void
SliceEditor::drawSlice(boost::shared_ptr<Slice> slice) {

	foreach (const util::point<int>& p, slice->getComponent()->getPixels())
		(*_sliceImage)(p.x - _region.minX, p.y - _region.minY) = 1.0;
}
