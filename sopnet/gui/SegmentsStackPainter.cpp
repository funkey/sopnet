#include "SegmentsStackPainter.h"

static logger::LogChannel segmentsstackpainterlog("segmentsstackpainterlog", "[SegmentsStackPainter] ");

SegmentsStackPainter::SegmentsStackPainter() :
	_prevSegments(boost::make_shared<Segments>()),
	_nextSegments(boost::make_shared<Segments>()),
	_section(0),
	_zScale(15) {}

void
SegmentsStackPainter::setSegments(boost::shared_ptr<Segments> segments) {

	LOG_DEBUG(segmentsstackpainterlog) << "got new segments" << std::endl;

	_segments = segments;

	_textures.clear();

	setCurrentSection(_section);
}

void
SegmentsStackPainter::setCurrentSection(unsigned int section) {

	_section = std::min(section, _segments->getNumInterSectionIntervals());

	// update the sets of previous and next segments

	int prevInterval = _section;

	_prevSegments->clear();

	if (prevInterval >= 0) {

		_prevSegments->addAll(_segments->getEnds(prevInterval));
		_prevSegments->addAll(_segments->getContinuations(prevInterval));
		_prevSegments->addAll(_segments->getBranches(prevInterval));
	}

	int nextInterval = _section + 1;

	_nextSegments->clear();

	if (nextInterval < _segments->getNumInterSectionIntervals()) {

		_nextSegments->addAll(_segments->getEnds(nextInterval));
		_nextSegments->addAll(_segments->getContinuations(nextInterval));
		_nextSegments->addAll(_segments->getBranches(nextInterval));
	}

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

	setSize(size);

	LOG_DEBUG(segmentsstackpainterlog) << "current section set to " << _section << std::endl;
	LOG_DEBUG(segmentsstackpainterlog) << "current size set to " << size << std::endl;
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

	glCheck(glColor4f(red, green, blue, alpha));

	_textures.get(slice.getId())->bind();

	glBegin(GL_QUADS);

	const util::rect<double>& bb = slice.getComponent()->getBoundingBox();

	// left side
	glTexCoord2d(1.0, 0.0); glNormal3d(0, 0, -1); glVertex3d(bb.maxX, bb.minY, z);
	glTexCoord2d(1.0, 1.0); glNormal3d(0, 0, -1); glVertex3d(bb.maxX, bb.maxY, z);
	glTexCoord2d(0.0, 1.0); glNormal3d(0, 0, -1); glVertex3d(bb.minX, bb.maxY, z);
	glTexCoord2d(0.0, 0.0); glNormal3d(0, 0, -1); glVertex3d(bb.minX, bb.minY, z);

	// right side
	glTexCoord2d(0.0, 0.0); glNormal3d(0, 0, 1); glVertex3d(bb.minX, bb.minY, z);
	glTexCoord2d(0.0, 1.0); glNormal3d(0, 0, 1); glVertex3d(bb.minX, bb.maxY, z);
	glTexCoord2d(1.0, 1.0); glNormal3d(0, 0, 1); glVertex3d(bb.maxX, bb.maxY, z);
	glTexCoord2d(1.0, 0.0); glNormal3d(0, 0, 1); glVertex3d(bb.maxX, bb.minY, z);

	glCheck(glEnd());
}
