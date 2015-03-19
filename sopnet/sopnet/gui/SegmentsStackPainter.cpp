#include <boost/lexical_cast.hpp>

#include <gui/TextPainter.h>
#include "SegmentsStackPainter.h"

static logger::LogChannel segmentsstackpainterlog("segmentsstackpainterlog", "[SegmentsStackPainter] ");

SegmentsStackPainter::SegmentsStackPainter(double gap) :
	_prevSegments(boost::make_shared<Segments>()),
	_nextSegments(boost::make_shared<Segments>()),
	_closestPrevSegment(0),
	_closestNextSegment(0),
	_section(0),
	_showPrev(false),
	_showNext(true),
	_showEnds(false),
	_showContinuations(true),
	_showBranches(false),
	_showSliceIds(true),
	_focus(0, 0),
	_zScale(15),
	_gap(gap) {}

void
SegmentsStackPainter::setSegments(boost::shared_ptr<Segments> segments) {

	LOG_DEBUG(segmentsstackpainterlog) << "got new segments" << std::endl;

	_segments = segments;

	_textures.clear();

	setCurrentSection(_section);
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
SegmentsStackPainter::showSliceIds(bool show) {

	_showSliceIds = show;
}

void
SegmentsStackPainter::setFocus(const util::point<double>& focus) {

	LOG_DEBUG(segmentsstackpainterlog) << "setting focus to " << focus << std::endl;

	_focus = focus;

	// get closest segments to focus

	// create a dummy pixel list with only the focus point in it
	boost::shared_ptr<ConnectedComponent::pixel_list_type> dummyPixelList = boost::make_shared<ConnectedComponent::pixel_list_type>(1);
	dummyPixelList->add(util::point<unsigned int>(focus));

	// create a connected component for this list
	boost::shared_ptr<ConnectedComponent> dummyComponent =
			boost::make_shared<ConnectedComponent>(
					boost::shared_ptr<Image>(),
					0,
					dummyPixelList,
					dummyPixelList->begin(),
					dummyPixelList->end());

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

	unsigned int height = size.height();

	size.minY -= height + _gap;
	size.maxY += height + _gap;

	setSize(size);

	LOG_DEBUG(segmentsstackpainterlog) << "current section set to " << _section << std::endl;
	LOG_DEBUG(segmentsstackpainterlog) << "current size set to " << size << std::endl;
}

void
SegmentsStackPainter::updateVisibleSegments() {

	// update the sets of previous and next segments

	_prevSegments->clear();
	_nextSegments->clear();

	if (_showPrev) {

		if (_showEnds && _closestPrevEndSegments.size() > _closestPrevSegment)
			_prevSegments->add(_closestPrevEndSegments[_closestPrevSegment].first);
		else if (_showContinuations && _closestPrevContinuationSegments.size() > _closestPrevSegment)
			_prevSegments->add(_closestPrevContinuationSegments[_closestPrevSegment].first);
		else if (_showBranches && _closestPrevBranchSegments.size() > _closestPrevSegment)
			_prevSegments->add(_closestPrevBranchSegments[_closestPrevSegment].first);
	}

	if (_showNext) {

		if (_showEnds && _closestNextEndSegments.size() > _closestNextSegment)
			_nextSegments->add(_closestNextEndSegments[_closestNextSegment].first);
		else if (_showContinuations && _closestNextContinuationSegments.size() > _closestNextSegment)
			_nextSegments->add(_closestNextContinuationSegments[_closestNextSegment].first);
		else if (_showBranches && _closestNextBranchSegments.size() > _closestNextSegment)
			_nextSegments->add(_closestNextBranchSegments[_closestNextSegment].first);
	}
}

util::rect<double>
SegmentsStackPainter::sizeAddSlice(const util::rect<double>& currentSize, const Slice& slice) {

	if (currentSize.width() == 0 && currentSize.height() == 0)
		return slice.getComponent()->getBoundingBox();

	util::rect<double> size;

	size.minX = std::min(currentSize.minX, (double)slice.getComponent()->getBoundingBox().minX);
	size.minY = std::min(currentSize.minY, (double)slice.getComponent()->getBoundingBox().minY);
	size.maxX = std::max(currentSize.maxX, (double)slice.getComponent()->getBoundingBox().maxX);
	size.maxY = std::max(currentSize.maxY, (double)slice.getComponent()->getBoundingBox().maxY);

	return size;
}

bool
SegmentsStackPainter::draw(
		const util::rect<double>&  roi,
		const util::point<double>& resolution) {

	gui::OpenGl::Guard guard;

	LOG_ALL(segmentsstackpainterlog) << "redrawing section " << _section << std::endl;

	// from previous section

	foreach (boost::shared_ptr<EndSegment> end, _prevSegments->getEnds()) {

		if (end->getDirection() == Left) {

			LOG_ALL(segmentsstackpainterlog) << "drawing an end..." << std::endl;

			drawSlice(*end->getSlice(), 0.0, 1.0, 0.0, 0.0, 0.85, roi, resolution);

			LOG_ALL(segmentsstackpainterlog) << "done" << std::endl;
		}
	}

	foreach (boost::shared_ptr<ContinuationSegment> continuation, _prevSegments->getContinuations()) {

		LOG_ALL(segmentsstackpainterlog) << "drawing a continuation..." << std::endl;

		drawSlice(
				*continuation->getSourceSlice(),
				(continuation->getDirection() == Right ? -_zScale : 0.0),
				0.0, 1.0, 0.0, 0.85,
				roi, resolution);

		drawSlice(
				*continuation->getTargetSlice(),
				(continuation->getDirection() == Left  ? -_zScale : 0.0),
				0.0, 1.0, 0.0, 0.85,
				roi, resolution);

		LOG_ALL(segmentsstackpainterlog) << "done" << std::endl;
	}

	foreach (boost::shared_ptr<BranchSegment> branch, _prevSegments->getBranches()) {

		LOG_ALL(segmentsstackpainterlog) << "drawing a branch..." << std::endl;

		drawSlice(
				*branch->getSourceSlice(),
				(branch->getDirection() == Right ? -_zScale : 0.0),
				0.0, 0.0, 1.0, 0.85,
				roi, resolution);

		drawSlice(
				*branch->getTargetSlice1(),
				(branch->getDirection() == Left  ? -_zScale : 0.0),
				0.0, 0.0, 1.0, 0.85,
				roi, resolution);

		drawSlice(
				*branch->getTargetSlice2(),
				(branch->getDirection() == Left  ? -_zScale : 0.0),
				0.0, 0.0, 1.0, 0.85,
				roi, resolution);

		LOG_ALL(segmentsstackpainterlog) << "done" << std::endl;
	}

	// to next section

	foreach (boost::shared_ptr<EndSegment> end, _nextSegments->getEnds()) {

		if (end->getDirection() == Right) {

			LOG_ALL(segmentsstackpainterlog) << "drawing an end..." << std::endl;

			drawSlice(*end->getSlice(), 0.0, 1.0, 0.0, 0.0, 0.85, roi, resolution);

			LOG_ALL(segmentsstackpainterlog) << "done" << std::endl;
		}
	}

	foreach (boost::shared_ptr<ContinuationSegment> continuation, _nextSegments->getContinuations()) {

		LOG_ALL(segmentsstackpainterlog) << "drawing a continuation..." << std::endl;

		drawSlice(
				*continuation->getSourceSlice(),
				(continuation->getDirection() == Left ? _zScale : 0.0),
				0.0, 1.0, 0.0, 0.85,
				roi, resolution);

		drawSlice(
				*continuation->getTargetSlice(),
				(continuation->getDirection() == Right ? _zScale : 0.0),
				0.0, 1.0, 0.0, 0.85,
				roi, resolution);

		LOG_ALL(segmentsstackpainterlog) << "done" << std::endl;
	}

	foreach (boost::shared_ptr<BranchSegment> branch, _nextSegments->getBranches()) {

		LOG_ALL(segmentsstackpainterlog) << "drawing a branch..." << std::endl;

		drawSlice(
				*branch->getSourceSlice(),
				(branch->getDirection() == Left  ? _zScale : 0.0),
				0.0, 0.0, 1.0, 0.85,
				roi, resolution);

		drawSlice(
				*branch->getTargetSlice1(),
				(branch->getDirection() == Right ? _zScale : 0.0),
				0.0, 0.0, 1.0, 0.85,
				roi, resolution);

		drawSlice(
				*branch->getTargetSlice2(),
				(branch->getDirection() == Right ? _zScale : 0.0),
				0.0, 0.0, 1.0, 0.85,
				roi, resolution);

		LOG_ALL(segmentsstackpainterlog) << "done" << std::endl;
	}

	return false;
}

void
SegmentsStackPainter::drawSlice(
		const Slice& slice,
		double z,
		double red, double green, double blue,
		double alpha,
		const util::rect<double>&  roi,
		const util::point<double>& resolution) {

	glCheck(glColor4f(red, green, blue, alpha));

	glCheck(glEnable(GL_TEXTURE_2D));

	glCheck(glEnable(GL_BLEND));
	glCheck(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

	glCheck(glEnable(GL_CULL_FACE));
	glCheck(glEnable(GL_LIGHTING));
	glCheck(glEnable(GL_LIGHT0));
	glCheck(glEnable(GL_COLOR_MATERIAL));
	glCheck(glDisable(GL_DEPTH_TEST));

	_textures.get(slice.getId())->bind();

	glBegin(GL_QUADS);

	const util::rect<double>& bb = slice.getComponent()->getBoundingBox();

	double offset = 0;

	// draw the segment in the previous or next section, instead of in front of 
	// current section
	if (z < 0) {

		offset = _sectionHeight + _gap;
		z = 0;

	} else if (z > 0) {

		offset = -_sectionHeight - _gap;
		z = 0;
	}

	// right side
	glTexCoord2d(0.0, 0.0); glNormal3d(0, 0, 1); glVertex3d(bb.minX, bb.minY + offset, 0);
	glTexCoord2d(0.0, 1.0); glNormal3d(0, 0, 1); glVertex3d(bb.minX, bb.maxY + offset, 0);
	glTexCoord2d(1.0, 1.0); glNormal3d(0, 0, 1); glVertex3d(bb.maxX, bb.maxY + offset, 0);
	glTexCoord2d(1.0, 0.0); glNormal3d(0, 0, 1); glVertex3d(bb.maxX, bb.minY + offset, 0);

	glCheck(glEnd());

	if (_showSliceIds) {

		gui::TextPainter idPainter(boost::lexical_cast<std::string>(slice.getId()));
		idPainter.setTextSize(10.0);
		idPainter.setTextColor(1.0 - red, 1.0 - green, 1.0 - blue);

		double x = slice.getComponent()->getCenter().x;
		double y = slice.getComponent()->getCenter().y + offset;

		glPushMatrix();
		glTranslatef(x, y, 0);
		idPainter.draw(roi - util::point<double>(x, y), resolution);
		glPopMatrix();
	}
}

void
SegmentsStackPainter::getVisibleSegments(Segments& segments) {

	segments.clear();

	segments.addAll(_prevSegments->getEnds());
	segments.addAll(_prevSegments->getContinuations());
	segments.addAll(_prevSegments->getBranches());
	segments.addAll(_nextSegments->getEnds());
	segments.addAll(_nextSegments->getContinuations());
	segments.addAll(_nextSegments->getBranches());
}
