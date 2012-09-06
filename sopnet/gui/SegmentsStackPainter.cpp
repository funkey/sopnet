#include "SegmentsStackPainter.h"

static logger::LogChannel segmentsstackpainterlog("segmentsstackpainterlog", "[SegmentsStackPainter] ");

SegmentsStackPainter::SegmentsStackPainter(bool onlyOneSegment) :
	_prevSegments(boost::make_shared<Segments>()),
	_nextSegments(boost::make_shared<Segments>()),
	_section(0),
	_onlyOneSegment(onlyOneSegment),
	_showNext(true),
	_showPrev(onlyOneSegment ? false : true),
	_showEnds(onlyOneSegment ? false : true),
	_showContinuations(true),
	_showBranches(onlyOneSegment ? false : true),
	_closestPrevSegment(0),
	_closestNextSegment(0),
	_focus(0, 0),
	_zScale(15) {}

void
SegmentsStackPainter::setSegments(boost::shared_ptr<Segments> segments) {

	LOG_DEBUG(segmentsstackpainterlog) << "got new segments" << std::endl;

	_segments = segments;

	if (!_onlyOneSegment)
		assignColors();

	_textures.clear();

	setCurrentSection(_section);
}

bool
SegmentsStackPainter::toggleShowOnlyOneSegment() {

	LOG_DEBUG(segmentsstackpainterlog) << "toggling only-one-segment mode" << std::endl;

	_onlyOneSegment = !_onlyOneSegment;

	if (!_onlyOneSegment)
		assignColors();

	updateVisibleSegments();

	return _onlyOneSegment;
}

void
SegmentsStackPainter::showPrev(bool show) {

	LOG_DEBUG(segmentsstackpainterlog) << "showing previous: " << show << std::endl;

	_showPrev = show;

	updateVisibleSegments();
}

void
SegmentsStackPainter::showNext(bool show) {

	LOG_DEBUG(segmentsstackpainterlog) << "showing next: " << show << std::endl;

	_showNext = show;

	updateVisibleSegments();
}

void
SegmentsStackPainter::showEnds(bool show) {

	LOG_DEBUG(segmentsstackpainterlog) << "showing ends: " << show << std::endl;

	_showEnds = show;

	updateVisibleSegments();
}

void
SegmentsStackPainter::showContinuations(bool show) {

	LOG_DEBUG(segmentsstackpainterlog) << "showing continuations: " << show << std::endl;

	_showContinuations = show;

	updateVisibleSegments();
}

void
SegmentsStackPainter::showBranches(bool show) {

	LOG_DEBUG(segmentsstackpainterlog) << "showing branches: " << show << std::endl;

	_showBranches = show;

	updateVisibleSegments();
}

void
SegmentsStackPainter::setFocus(const util::point<double>& focus) {

	LOG_DEBUG(segmentsstackpainterlog) << "setting focus to " << focus << std::endl;

	_focus = focus;

	// get closest segments to focus

	// create a dummy pixel list with only the focus point in it
	boost::shared_ptr<ConnectedComponent::pixel_list_type> dummyPixelList = boost::make_shared<ConnectedComponent::pixel_list_type>();
	dummyPixelList->push_back(util::point<unsigned int>(focus));

	// create a connected component for this list
	boost::shared_ptr<ConnectedComponent> dummyComponent = boost::make_shared<ConnectedComponent>(boost::shared_ptr<Image>(), 0, dummyPixelList, 0, 1);

	// create a dummy slice from the component
	boost::shared_ptr<Slice> dummySlice = boost::make_shared<Slice>(0, _section, dummyComponent);

	// create dummy end segments for this slice
	boost::shared_ptr<EndSegment> prevEnd = boost::make_shared<EndSegment>(0, Left, dummySlice);
	boost::shared_ptr<EndSegment> nextEnd = boost::make_shared<EndSegment>(0, Right, dummySlice);

	// find closest ends to dummy ends
	_closestPrevEndSegments = _segments->findEnds(prevEnd, 1000000000);
	_closestNextEndSegments = _segments->findEnds(nextEnd, 1000000000);

	// create dummy continuation segments for this slice
	boost::shared_ptr<ContinuationSegment> prevContinuation = boost::make_shared<ContinuationSegment>(0, Left, dummySlice, dummySlice);
	boost::shared_ptr<ContinuationSegment> nextContinuation = boost::make_shared<ContinuationSegment>(0, Right, dummySlice, dummySlice);

	// find closest continuations to dummy continuations
	_closestPrevContinuationSegments = _segments->findContinuations(prevContinuation, 1000000000);
	_closestNextContinuationSegments = _segments->findContinuations(nextContinuation, 1000000000);

	// create dummy branch segments for this slice
	boost::shared_ptr<BranchSegment> prevBranch = boost::make_shared<BranchSegment>(0, Left, dummySlice, dummySlice, dummySlice);
	boost::shared_ptr<BranchSegment> nextBranch = boost::make_shared<BranchSegment>(0, Right, dummySlice, dummySlice, dummySlice);

	// find closest branches to dummy branches
	_closestPrevBranchSegments = _segments->findBranches(prevBranch, 1000000000);
	_closestNextBranchSegments = _segments->findBranches(nextBranch, 1000000000);

	_closestPrevSegment = 0;
	_closestNextSegment = 0;

	LOG_DEBUG(segmentsstackpainterlog) << "found " << _closestPrevEndSegments.size() << " prev end segments" << std::endl;
	LOG_DEBUG(segmentsstackpainterlog) << "found " << _closestNextEndSegments.size() << " next end segments" << std::endl;
	LOG_DEBUG(segmentsstackpainterlog) << "found " << _closestPrevContinuationSegments.size() << " prev continuation segments" << std::endl;
	LOG_DEBUG(segmentsstackpainterlog) << "found " << _closestNextContinuationSegments.size() << " next continuation segments" << std::endl;
	LOG_DEBUG(segmentsstackpainterlog) << "found " << _closestPrevBranchSegments.size() << " prev branch segments" << std::endl;
	LOG_DEBUG(segmentsstackpainterlog) << "found " << _closestNextBranchSegments.size() << " next branch segments" << std::endl;

	updateVisibleSegments();
}

void
SegmentsStackPainter::nextSegment() {

	if (!_onlyOneSegment)
		return;

	LOG_DEBUG(segmentsstackpainterlog) << "old prev segment: " << _closestPrevSegment << std::endl;
	LOG_DEBUG(segmentsstackpainterlog) << "old next segment: " << _closestNextSegment << std::endl;

	unsigned int numSegments = 0;

	if (_showEnds) {

		if (_showPrev)
			numSegments = _closestPrevEndSegments.size();
		else if (_showNext)
			numSegments = _closestNextEndSegments.size();
	}

	if (_showContinuations) {

		if (_showPrev)
			numSegments = _closestPrevContinuationSegments.size();
		else if (_showNext)
			numSegments = _closestNextContinuationSegments.size();
	}

	if (_showBranches) {

		if (_showPrev)
			numSegments = _closestPrevBranchSegments.size();
		else if (_showNext)
			numSegments = _closestNextBranchSegments.size();
	}

	if (numSegments == 0)
		return;

	if (_showPrev) {

		if (_closestPrevSegment < numSegments - 1)
			_closestPrevSegment++;

	} else if (_showNext) {

		if (_closestNextSegment < numSegments - 1)
			_closestNextSegment++;
	}

	LOG_DEBUG(segmentsstackpainterlog) << "current prev segment: " << _closestPrevSegment << std::endl;
	LOG_DEBUG(segmentsstackpainterlog) << "current next segment: " << _closestNextSegment << std::endl;

	updateVisibleSegments();
}

void
SegmentsStackPainter::prevSegment() {

	if (!_onlyOneSegment)
		return;

	LOG_DEBUG(segmentsstackpainterlog) << "old prev segment: " << _closestPrevSegment << std::endl;
	LOG_DEBUG(segmentsstackpainterlog) << "old next segment: " << _closestNextSegment << std::endl;

	if (_showPrev) {

		if (_closestPrevSegment > 0)
			_closestPrevSegment--;

	} else if (_showNext) {

		if (_closestNextSegment > 0)
			_closestNextSegment--;
	}

	LOG_DEBUG(segmentsstackpainterlog) << "current prev segment: " << _closestPrevSegment << std::endl;
	LOG_DEBUG(segmentsstackpainterlog) << "current next segment: " << _closestNextSegment << std::endl;

	updateVisibleSegments();
}

void
SegmentsStackPainter::assignColors() {

	LOG_DEBUG(segmentsstackpainterlog) << "assigning colors" << std::endl;

	_colors.clear();

	// identify connected segments

	_slices.clear();

	// collect all end slices
	foreach (boost::shared_ptr<EndSegment> end, _segments->getEnds())
		addSlice(end->getSlice()->getId());

	// identify slices belonging to the same neuron
	foreach (boost::shared_ptr<ContinuationSegment> continuation, _segments->getContinuations()) {

		mergeSlices(
				continuation->getSourceSlice()->getId(),
				continuation->getTargetSlice()->getId());
	}

	foreach (boost::shared_ptr<BranchSegment> branch, _segments->getBranches()) {

		mergeSlices(
				branch->getSourceSlice()->getId(),
				branch->getTargetSlice1()->getId());

		mergeSlices(
				branch->getSourceSlice()->getId(),
				branch->getTargetSlice2()->getId());

		mergeSlices(
				branch->getTargetSlice1()->getId(),
				branch->getTargetSlice2()->getId());
	}

	// assign colors to all connected sets of slices

	unsigned int sliceId;
	std::set<unsigned int> sameNeuronSlices;

	foreach (boost::tie(sliceId, sameNeuronSlices), _slices) {

		// draw a random color
		double r = (double)rand()/RAND_MAX;
		double g = (double)rand()/RAND_MAX;
		double b = (double)rand()/RAND_MAX;

		_colors[sliceId][0] = r;
		_colors[sliceId][1] = g;
		_colors[sliceId][2] = b;

		foreach (unsigned int id, sameNeuronSlices) {

			_colors[id][0] = r;
			_colors[id][1] = g;
			_colors[id][2] = b;
		}
	 }

	LOG_DEBUG(segmentsstackpainterlog) << "done" << std::endl;
}

void
SegmentsStackPainter::addSlice(unsigned int slice) {

	_slices[slice].insert(slice);
}

void
SegmentsStackPainter::mergeSlices(unsigned int slice1, unsigned int slice2) {

	// make sure we have partner sets for the given slices
	addSlice(slice1);
	addSlice(slice2);

	// get all partners of slice2 and add them to the partners of slice1
	foreach (unsigned int id, _slices[slice2])
		_slices[slice1].insert(id);

	// slice1 is now the only one, who knows all its partners

	// the partners of slice1 might not know about slice2 or any of its partners

	// for each partner of slice1, add all partners of slice1 to the partner list
	foreach (unsigned int id, _slices[slice1]) {

		foreach (unsigned int partner, _slices[slice1])
			_slices[id].insert(partner);
	}
}

void
SegmentsStackPainter::setCurrentSection(unsigned int section) {

	_section = std::min(section, _segments->getNumInterSectionIntervals());

	_closestNextSegment = 0;
	_closestPrevSegment = 0;

	setFocus(_focus);

	updateVisibleSegments();

	// get the size of the painter and load missing textures
	util::rect<double> size(0, 0, 0, 0);

	foreach (boost::shared_ptr<EndSegment> end, _segments->getEnds()) {

		size = sizeAddSlice(size, *end->getSlice());

		_textures.load(*end->getSlice());
	}

	foreach (boost::shared_ptr<ContinuationSegment> continuation, _segments->getContinuations()) {

		size = sizeAddSlice(size, *continuation->getSourceSlice());
		size = sizeAddSlice(size, *continuation->getTargetSlice());

		_textures.load(*continuation->getSourceSlice());
		_textures.load(*continuation->getTargetSlice());
	}

	foreach (boost::shared_ptr<BranchSegment> branch, _segments->getBranches()) {

		size = sizeAddSlice(size, *branch->getSourceSlice());
		size = sizeAddSlice(size, *branch->getTargetSlice1());
		size = sizeAddSlice(size, *branch->getTargetSlice2());

		_textures.load(*branch->getSourceSlice());
		_textures.load(*branch->getTargetSlice1());
		_textures.load(*branch->getTargetSlice2());
	}

	_sectionHeight = size.height();

	// for the only-one-segment mode, we show the segment partner slices above
	// and below the current section -- therefor we are three times bigger
	if (_onlyOneSegment) {

		unsigned int height = size.height();

		size.minY -= height;
		size.maxY += height;
	}

	setSize(size);

	LOG_DEBUG(segmentsstackpainterlog) << "current section set to " << _section << std::endl;
	LOG_DEBUG(segmentsstackpainterlog) << "current size set to " << size << std::endl;
}

void
SegmentsStackPainter::updateVisibleSegments() {

	// update the sets of previous and next segments

	int prevInterval = _section;
	int nextInterval = _section + 1;

	_prevSegments->clear();
	_nextSegments->clear();

	if (_onlyOneSegment) {

		if (_showPrev)
			if (_showEnds && _closestPrevEndSegments.size() > 0)
				_prevSegments->add(_closestPrevEndSegments[_closestPrevSegment]);
			else if (_showContinuations && _closestPrevContinuationSegments.size() > 0)
				_prevSegments->add(_closestPrevContinuationSegments[_closestPrevSegment]);
			else if (_showBranches && _closestPrevBranchSegments.size() > 0)
				_prevSegments->add(_closestPrevBranchSegments[_closestPrevSegment]);

		if (_showNext)
			if (_showEnds && _closestNextEndSegments.size() > 0)
				_nextSegments->add(_closestNextEndSegments[_closestNextSegment]);
			else if (_showContinuations && _closestNextContinuationSegments.size() > 0)
				_nextSegments->add(_closestNextContinuationSegments[_closestNextSegment]);
			else if (_showBranches && _closestNextBranchSegments.size() > 0)
				_nextSegments->add(_closestNextBranchSegments[_closestNextSegment]);

	} else {

		if (prevInterval >= 0) {

			if (_showEnds)
				_prevSegments->addAll(_segments->getEnds(prevInterval));
			if (_showContinuations)
				_prevSegments->addAll(_segments->getContinuations(prevInterval));
			if (_showBranches)
				_prevSegments->addAll(_segments->getBranches(prevInterval));
		}

		if (nextInterval < _segments->getNumInterSectionIntervals()) {

			if (_showEnds)
				_nextSegments->addAll(_segments->getEnds(nextInterval));
			if (_showContinuations)
				_nextSegments->addAll(_segments->getContinuations(nextInterval));
			if (_showBranches)
				_nextSegments->addAll(_segments->getBranches(nextInterval));
		}
	}
}

util::rect<double>
SegmentsStackPainter::sizeAddSlice(const util::rect<double>& currentSize, const Slice& slice) {

	if (currentSize.width() == 0 && currentSize.height() == 0)
		return slice.getComponent()->getBoundingBox();

	util::rect<double> size;

	size.minX = std::min(currentSize.minX, slice.getComponent()->getBoundingBox().minX);
	size.minY = std::min(currentSize.minY, slice.getComponent()->getBoundingBox().minY);
	size.maxX = std::max(currentSize.maxX, slice.getComponent()->getBoundingBox().maxX);
	size.maxY = std::max(currentSize.maxY, slice.getComponent()->getBoundingBox().maxY);

	return size;
}

void
SegmentsStackPainter::draw(
		const util::rect<double>&  roi,
		const util::point<double>& resolution) {

	// set up lighting
	GLfloat ambient[4] = { 0, 0, 0, 1 };
	glCheck(glLightfv(GL_LIGHT0, GL_AMBIENT, ambient));
	GLfloat diffuse[4] = { 0.1, 0.1, 0.1, 1 };
	glCheck(glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse));
	GLfloat specular[4] = { 0.1, 0.1, 0.1, 1 };
	glCheck(glLightfv(GL_LIGHT0, GL_SPECULAR, specular));

	glCheck(glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular));
	GLfloat emission[4] = { 0, 0, 0, 1 };
	glCheck(glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emission));

	// enable alpha blending
	glCheck(glEnable(GL_BLEND));
	glCheck(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

	glCheck(glEnable(GL_CULL_FACE));
	glCheck(glEnable(GL_LIGHTING));
	glCheck(glEnable(GL_LIGHT0));
	glCheck(glEnable(GL_COLOR_MATERIAL));

	LOG_ALL(segmentsstackpainterlog) << "redrawing section " << _section << std::endl;

	// from previous section

	foreach (boost::shared_ptr<EndSegment> end, _prevSegments->getEnds()) {

		if (end->getDirection() == Left) {

			LOG_ALL(segmentsstackpainterlog) << "drawing an end..." << std::endl;

			drawSlice(*end->getSlice(), 0.0, 1.0, 0.0, 0.0, 0.75);

			LOG_ALL(segmentsstackpainterlog) << "done" << std::endl;
		}
	}

	foreach (boost::shared_ptr<ContinuationSegment> continuation, _prevSegments->getContinuations()) {

		LOG_ALL(segmentsstackpainterlog) << "drawing a continuation..." << std::endl;

		drawSlice(
				*continuation->getSourceSlice(),
				(continuation->getDirection() == Right ? -_zScale : 0.0),
				0.0, 1.0, 0.0,
				(continuation->getDirection() == Right ? 0.25 : 0.75));

		drawSlice(
				*continuation->getTargetSlice(),
				(continuation->getDirection() == Left  ? -_zScale : 0.0),
				0.0, 1.0, 0.0,
				(continuation->getDirection() == Left  ? 0.25 : 0.75));

		LOG_ALL(segmentsstackpainterlog) << "done" << std::endl;
	}

	foreach (boost::shared_ptr<BranchSegment> branch, _prevSegments->getBranches()) {

		LOG_ALL(segmentsstackpainterlog) << "drawing a branch..." << std::endl;

		drawSlice(
				*branch->getSourceSlice(),
				(branch->getDirection() == Right ? -_zScale : 0.0),
				0.0, 0.0, 1.0,
				(branch->getDirection() == Right ? 0.25 : 0.75));

		drawSlice(
				*branch->getTargetSlice1(),
				(branch->getDirection() == Left  ? -_zScale : 0.0),
				0.0, 0.0, 1.0,
				(branch->getDirection() == Left ? 0.25 : 0.75));

		drawSlice(
				*branch->getTargetSlice2(),
				(branch->getDirection() == Left  ? -_zScale : 0.0),
				0.0, 0.0, 1.0,
				(branch->getDirection() == Left ? 0.25 : 0.75));

		LOG_ALL(segmentsstackpainterlog) << "done" << std::endl;
	}

	// to next section

	foreach (boost::shared_ptr<EndSegment> end, _nextSegments->getEnds()) {

		if (end->getDirection() == Right) {

			LOG_ALL(segmentsstackpainterlog) << "drawing an end..." << std::endl;

			drawSlice(*end->getSlice(), 0.0, 1.0, 0.0, 0.0, 0.75);

			LOG_ALL(segmentsstackpainterlog) << "done" << std::endl;
		}
	}

	foreach (boost::shared_ptr<ContinuationSegment> continuation, _nextSegments->getContinuations()) {

		LOG_ALL(segmentsstackpainterlog) << "drawing a continuation..." << std::endl;

		drawSlice(
				*continuation->getSourceSlice(),
				(continuation->getDirection() == Left ? _zScale : 0.0),
				0.0, 1.0, 0.0,
				(continuation->getDirection() == Left ? 0.25 : 0.75));

		drawSlice(
				*continuation->getTargetSlice(),
				(continuation->getDirection() == Right ? _zScale : 0.0),
				0.0, 1.0, 0.0,
				(continuation->getDirection() == Right ? 0.25 : 0.75));

		LOG_ALL(segmentsstackpainterlog) << "done" << std::endl;
	}

	foreach (boost::shared_ptr<BranchSegment> branch, _nextSegments->getBranches()) {

		LOG_ALL(segmentsstackpainterlog) << "drawing a branch..." << std::endl;

		drawSlice(
				*branch->getSourceSlice(),
				(branch->getDirection() == Left  ? _zScale : 0.0),
				0.0, 0.0, 1.0,
				(branch->getDirection() == Left ? 0.25 : 0.75));

		drawSlice(
				*branch->getTargetSlice1(),
				(branch->getDirection() == Right ? _zScale : 0.0),
				0.0, 0.0, 1.0,
				(branch->getDirection() == Right ? 0.25 : 0.75));

		drawSlice(
				*branch->getTargetSlice2(),
				(branch->getDirection() == Right ? _zScale : 0.0),
				0.0, 0.0, 1.0,
				(branch->getDirection() == Right ? 0.25 : 0.75));

		LOG_ALL(segmentsstackpainterlog) << "done" << std::endl;
	}

	glCheck(glDisable(GL_BLEND));
	glCheck(glDisable(GL_LIGHTING));
	glCheck(glDisable(GL_CULL_FACE));
}

void
SegmentsStackPainter::drawSlice(
		const Slice& slice,
		double z,
		double red, double green, double blue,
		double alpha) {

	double section = slice.getSection();

	boost::array<double, 3> color;
	
	if (_onlyOneSegment) {

		color[0] = red;
		color[1] = green;
		color[2] = blue;

		alpha = 0.9;

	} else {
	
		color = _colors[slice.getId()];
	}

	glCheck(glColor4f(color[0], color[1], color[2], alpha));

	_textures.get(slice.getId())->bind();

	glBegin(GL_QUADS);

	const util::rect<double>& bb = slice.getComponent()->getBoundingBox();

	double offset = 0;

	if (_onlyOneSegment) {

		if (z < 0) {

			offset = _sectionHeight;
			z = 0;

		} else if (z > 0) {

			offset = -_sectionHeight;
			z = 0;
		}
	}

	// left side
	glTexCoord2d(1.0, 0.0); glNormal3d(0, 0, -1); glVertex3d(bb.maxX, bb.minY + offset, z);
	glTexCoord2d(1.0, 1.0); glNormal3d(0, 0, -1); glVertex3d(bb.maxX, bb.maxY + offset, z);
	glTexCoord2d(0.0, 1.0); glNormal3d(0, 0, -1); glVertex3d(bb.minX, bb.maxY + offset, z);
	glTexCoord2d(0.0, 0.0); glNormal3d(0, 0, -1); glVertex3d(bb.minX, bb.minY + offset, z);

	// right side
	glTexCoord2d(0.0, 0.0); glNormal3d(0, 0, 1); glVertex3d(bb.minX, bb.minY + offset, z);
	glTexCoord2d(0.0, 1.0); glNormal3d(0, 0, 1); glVertex3d(bb.minX, bb.maxY + offset, z);
	glTexCoord2d(1.0, 1.0); glNormal3d(0, 0, 1); glVertex3d(bb.maxX, bb.maxY + offset, z);
	glTexCoord2d(1.0, 0.0); glNormal3d(0, 0, 1); glVertex3d(bb.maxX, bb.minY + offset, z);

	glCheck(glEnd());
}

bool
SegmentsStackPainter::onlyOneSegment() {

	return _onlyOneSegment;
}
