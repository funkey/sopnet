#include <boost/range/adaptors.hpp>
#include <util/foreach.h>
#include <pipeline/Value.h>
#include <imageprocessing/ConnectedComponent.h>
#include <imageprocessing/ComponentTreeExtractor.h>
#include <sopnet/slices/SliceExtractor.h>
#include <sopnet/features/Overlap.h>
#include "SliceEditor.h"

logger::LogChannel sliceeditorlog("sliceeditorlog", "[SliceEditor] ");

SliceEditor::SliceEditor(const std::vector<boost::shared_ptr<Slice> >& initialSlices, unsigned int section, const util::rect<int>& region) :
	_initialSlices(initialSlices.begin(), initialSlices.end()),
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
					(*_sliceImage)(p.x + dx, p.y + dy) = (foreground ? 0.8 : 0.0);
}

SliceEdits
SliceEditor::finish() {

	SliceEdits edits;

	// create parameters suitable to extract connected components
	pipeline::Value<ComponentTreeExtractorParameters> cteParameters;
	cteParameters->minSize      = 0;
	cteParameters->maxSize      = 10000000;
	cteParameters->darkToBright = false;
	cteParameters->sameIntensityComponents = false;

	LOG_DEBUG(sliceeditorlog) << "extracting slices from current slice image" << std::endl;

	// create a SliceExtractor
	pipeline::Process<SliceExtractor<unsigned short> > sliceExtractor(_section, 1.0, 1.0, 1.0 /* dummy resolution */, false /* don't downsample */);

	// give it the section it has to process and our parameters
	sliceExtractor->setInput("membrane", _sliceImage);
	sliceExtractor->setInput("parameters", cteParameters);

	// get the slices in the current section
	pipeline::Value<Slices> extractedSlices = sliceExtractor->getOutput("slices");

	LOG_ALL(sliceeditorlog) << "found " << extractedSlices->size() << " slices" << std::endl;

	// translate them back to the initial slices
	Slices translatedSlices = *extractedSlices;
	translatedSlices.translate(_region.upperLeft());

	// find all perfectly overlapping slices and remove them (they have not 
	// changed)
	Overlap overlap(true, false);

	std::set<boost::shared_ptr<Slice> > changedInitialSlices(_initialSlices.begin(), _initialSlices.end());
	std::set<boost::shared_ptr<Slice> > newSlices(translatedSlices.begin(), translatedSlices.end());

	foreach (boost::shared_ptr<Slice> initialSlice, _initialSlices) {

		std::vector<boost::shared_ptr<Slice> > closeSlices = translatedSlices.find(initialSlice->getComponent()->getCenter(), 1);

		if (closeSlices.size() == 0)
			continue;

		// perfect overlap?
		if (overlap(*initialSlice, *closeSlices[0]) == 1) {

			// erase both slices
			std::set<boost::shared_ptr<Slice> >::iterator i;
			i = changedInitialSlices.find(initialSlice);
			changedInitialSlices.erase(i);
			i = newSlices.find(closeSlices[0]);
			newSlices.erase(i);
		}
	}

	// get all partially overlapping slices
	std::map<boost::shared_ptr<Slice>, std::vector<boost::shared_ptr<Slice> > > links;

	// init with empty link partners
	foreach (boost::shared_ptr<Slice> is, changedInitialSlices)
		links[is] = std::vector<boost::shared_ptr<Slice> >();

	// init with empty link partners
	foreach (boost::shared_ptr<Slice> ns, newSlices)
		links[ns] = std::vector<boost::shared_ptr<Slice> >();

	// find partially overlapping partners
	foreach (boost::shared_ptr<Slice> is, changedInitialSlices) {

		foreach (boost::shared_ptr<Slice> ns, newSlices) {

			double o = overlap(*is, *ns);

			LOG_ALL(sliceeditorlog)
					<< "overlap between slice " << is->getId()
					<< " and " << ns->getId()
					<< " is " << o << std::endl;

			// partial overlap
			if (o > 0) {

				links[is].push_back(ns);
				links[ns].push_back(is);
			}
		}
	}

	// turn overlap map into slice edits
	std::set<boost::shared_ptr<Slice> > processed;
	foreach (boost::shared_ptr<Slice> s, links | boost::adaptors::map_keys) {

		if (processed.count(s))
			continue;

		// get all slices s connects to
		std::set<boost::shared_ptr<Slice> > component;
		component.insert(s);
		bool done = false;
		while (!done) {

			done = true;
			foreach (boost::shared_ptr<Slice> t, component)
				foreach (boost::shared_ptr<Slice> u, links[t])
					done = done && !component.insert(u).second;
		}

		// turn it into an edit
		std::vector<boost::shared_ptr<Slice> > editInitialSlices;
		std::vector<boost::shared_ptr<Slice> > editNewSlices;

		LOG_ALL(sliceeditorlog) << "found component:" << std::endl;
		foreach (boost::shared_ptr<Slice> s, component)
			LOG_ALL(sliceeditorlog) << s->getId() << " ";
		LOG_ALL(sliceeditorlog) << std::endl;

		foreach (boost::shared_ptr<Slice> t, component) {

			if (_initialSlices.count(t))
				editInitialSlices.push_back(t);
			else
				editNewSlices.push_back(t);
		}

		LOG_ALL(sliceeditorlog)
				<< "found replacement of slices ";
		foreach (boost::shared_ptr<Slice> s, editInitialSlices)
			LOG_ALL(sliceeditorlog) << s->getId() << " ";
		LOG_ALL(sliceeditorlog)
				<< "with ";
		foreach (boost::shared_ptr<Slice> s, editNewSlices)
			LOG_ALL(sliceeditorlog) << s->getId() << " ";
		LOG_ALL(sliceeditorlog) << std::endl;

		edits.addReplacement(editInitialSlices, editNewSlices);

		processed.insert(component.begin(), component.end());
	}

	return edits;
}

void
SliceEditor::drawSlice(boost::shared_ptr<Slice> slice) {

	foreach (const util::point<int>& p, slice->getComponent()->getPixels())
		(*_sliceImage)(p.x - _region.minX, p.y - _region.minY) = 0.8;
}
