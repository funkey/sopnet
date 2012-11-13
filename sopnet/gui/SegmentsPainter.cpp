#include <boost/array.hpp>

#include <gui/OpenGl.h>
#include <imageprocessing/ConnectedComponent.h>
#include <sopnet/segments/EndSegment.h>
#include <sopnet/segments/ContinuationSegment.h>
#include <sopnet/segments/BranchSegment.h>
#include <util/foreach.h>
#include "SegmentsPainter.h"

logger::LogChannel segmentspainterlog("segmentspainterlog", "[SegmentsPainter] ");

SegmentsPainter::SegmentsPainter(std::string name) :
	RecordablePainter(name),
	_zScale(15),
	_showEnds(true),
	_showContinuations(true),
	_showBranches(true) {}

void
SegmentsPainter::setImageStack(boost::shared_ptr<ImageStack> imageStack) {

	_imageStack = imageStack;
}

void
SegmentsPainter::setSegments(boost::shared_ptr<Segments> segments) {

	LOG_ALL(segmentspainterlog) << getName() << ": got new segments:" << std::endl;

	LOG_ALL(segmentspainterlog) << getName() << ": " << segments->getEnds().size() << " ends" << std::endl;
	LOG_ALL(segmentspainterlog) << getName() << ": " << segments->getContinuations().size() << " continuations" << std::endl;
	LOG_ALL(segmentspainterlog) << getName() << ": " << segments->getBranches().size() << " branches" << std::endl;

	_segments = segments;

	loadTextures();

	_size = util::rect<double>(0, 0, 0, 0);

	updateRecording();

	LOG_ALL(segmentspainterlog) << getName() << ": size is " << _size << std::endl;

	setSize(_size);
}

void
SegmentsPainter::showEnds(bool show) {

	LOG_ALL(segmentspainterlog) << getName() << ": show ends == " << show << std::endl;

	_showEnds = show;

	loadTextures();

	_size = util::rect<double>(0, 0, 0, 0);

	updateRecording();

	LOG_ALL(segmentspainterlog) << getName() << ": size is " << _size << std::endl;

	setSize(_size);
}

void
SegmentsPainter::showContinuations(bool show) {

	LOG_ALL(segmentspainterlog) << getName() << ": show continuations == " << show << std::endl;

	_showContinuations = show;

	loadTextures();

	_size = util::rect<double>(0, 0, 0, 0);

	updateRecording();

	LOG_ALL(segmentspainterlog) << getName() << ": size is " << _size << std::endl;

	setSize(_size);
}

void
SegmentsPainter::showBranches(bool show) {

	LOG_ALL(segmentspainterlog) << getName() << ": show branches == " << show << std::endl;

	_showBranches = show;

	loadTextures();

	_size = util::rect<double>(0, 0, 0, 0);

	updateRecording();

	LOG_ALL(segmentspainterlog) << getName() << ": size is " << _size << std::endl;

	setSize(_size);
}

void
SegmentsPainter::loadTextures() {

	LOG_DEBUG(segmentspainterlog) << getName() << ": loading textures..." << std::endl;

	_textures.clear();

	if (_showEnds)
		foreach (boost::shared_ptr<EndSegment> segment, _segments->getEnds())
			loadTextures(*segment);

	if (_showContinuations)
		foreach (boost::shared_ptr<ContinuationSegment> segment, _segments->getContinuations())
			loadTextures(*segment);

	if (_showBranches)
		foreach (boost::shared_ptr<BranchSegment> segment, _segments->getBranches())
			loadTextures(*segment);

	LOG_DEBUG(segmentspainterlog) << getName() << ": all textures loaded..." << std::endl;
}

void
SegmentsPainter::loadTextures(const EndSegment& end) {

	loadTexture(*end.getSlice());
}

void
SegmentsPainter::loadTextures(const ContinuationSegment& continuation) {

	loadTexture(*continuation.getSourceSlice());
	loadTexture(*continuation.getTargetSlice());
}

void
SegmentsPainter::loadTextures(const BranchSegment& branch) {

	loadTexture(*branch.getSourceSlice());
	loadTexture(*branch.getTargetSlice1());
	loadTexture(*branch.getTargetSlice2());
}

void
SegmentsPainter::loadTexture(const Slice& slice) {

	_textures.load(slice, _imageStack);
}

void
SegmentsPainter::updateRecording() {

	LOG_DEBUG(segmentspainterlog) << getName() << ": updating recording" << std::endl;

	// make sure OpenGl operations are save
	gui::OpenGl::Guard guard;

	startRecording();

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

	// draw thicker lines and points
	glLineWidth(5.0);
	glEnable(GL_LINE_SMOOTH);

	// draw from front to back, facing to the right
	_leftSide  = false;
	_rightSide = true;
	for (int i = 0; i < _segments->getNumInterSectionIntervals(); i++) {

		if (_showEnds)
			foreach (boost::shared_ptr<EndSegment> segment, _segments->getEnds(i))
				draw(*segment);

		if (_showContinuations)
			foreach (boost::shared_ptr<ContinuationSegment> segment, _segments->getContinuations(i))
				draw(*segment);

		if (_showBranches)
			foreach (boost::shared_ptr<BranchSegment> segment, _segments->getBranches(i))
				draw(*segment);
	}

	// draw from back to front, facing to the left
	_leftSide  = true;
	_rightSide = false;
	for (int i = _segments->getNumInterSectionIntervals() - 1; i >= 0; i--) {

		if (_showEnds)
			foreach (boost::shared_ptr<EndSegment> segment, _segments->getEnds(i))
				draw(*segment);

		if (_showContinuations)
			foreach (boost::shared_ptr<ContinuationSegment> segment, _segments->getContinuations(i))
				draw(*segment);

		if (_showBranches)
			foreach (boost::shared_ptr<BranchSegment> segment, _segments->getBranches(i))
				draw(*segment);
	}

	glCheck(glDisable(GL_BLEND));
	glCheck(glDisable(GL_LIGHTING));
	glCheck(glDisable(GL_CULL_FACE));

	stopRecording();
}

void
SegmentsPainter::draw(const EndSegment& end) {

	glCheck(glEnable(GL_TEXTURE_2D));

	drawSlice(end.getSlice(), 1.0, 0.5, 0.5);

	glCheck(glDisable(GL_TEXTURE_2D));
}

void
SegmentsPainter::draw(const ContinuationSegment& continuation) {

	glCheck(glEnable(GL_TEXTURE_2D));

	drawSlice(continuation.getSourceSlice(), 0.5, 1.0, 0.5);
	drawSlice(continuation.getTargetSlice(), 0.5, 1.0, 0.5);

	util::point<double> center1 = continuation.getSourceSlice()->getComponent()->getCenter();
	util::point<double> center2 = continuation.getTargetSlice()->getComponent()->getCenter();

	unsigned int section1 = continuation.getSourceSlice()->getSection();
	unsigned int section2 = continuation.getTargetSlice()->getSection();

	glCheck(glDisable(GL_TEXTURE_2D));

	glBegin(GL_LINES);

	glCheck(glVertex3f(center1.x, center1.y, section1*_zScale));
	glCheck(glVertex3f(center2.x, center2.y, section2*_zScale));

	glCheck(glEnd());

	glBegin(GL_POINTS);

	glCheck(glVertex3f(center1.x, center1.y, section1*_zScale));
	glCheck(glVertex3f(center2.x, center2.y, section2*_zScale));

	glCheck(glEnd());
}

void
SegmentsPainter::draw(const BranchSegment& branch) {

	glCheck(glEnable(GL_TEXTURE_2D));

	drawSlice(branch.getSourceSlice(), 0.5, 0.5, 1.0);
	drawSlice(branch.getTargetSlice1(), 0.5, 0.5, 1.0);
	drawSlice(branch.getTargetSlice2(), 0.5, 0.5, 1.0);

	util::point<double> center1 = branch.getSourceSlice()->getComponent()->getCenter();
	util::point<double> center2 = branch.getTargetSlice1()->getComponent()->getCenter();
	util::point<double> center3 = branch.getTargetSlice2()->getComponent()->getCenter();

	unsigned int section1 = branch.getSourceSlice()->getSection();
	unsigned int section2 = branch.getTargetSlice1()->getSection();
	unsigned int section3 = branch.getTargetSlice2()->getSection();

	glCheck(glDisable(GL_TEXTURE_2D));

	glBegin(GL_LINES);

	glVertex3f(center1.x, center1.y, section1*_zScale);
	glVertex3f(center2.x, center2.y, section2*_zScale);

	glCheck(glEnd());

	glBegin(GL_POINTS);

	glVertex3f(center1.x, center1.y, section1*_zScale);
	glVertex3f(center2.x, center2.y, section2*_zScale);

	glCheck(glEnd());

	glBegin(GL_LINES);

	glVertex3f(center1.x, center1.y, section1*_zScale);
	glVertex3f(center3.x, center3.y, section3*_zScale);

	glCheck(glEnd());

	glBegin(GL_POINTS);

	glVertex3f(center1.x, center1.y, section1*_zScale);
	glVertex3f(center3.x, center3.y, section3*_zScale);

	glCheck(glEnd());
}

void
SegmentsPainter::drawSlice(
		boost::shared_ptr<Slice> slice,
		double red, double green, double blue) {

	double section = slice->getSection();

	LOG_ALL(segmentspainterlog)
			<< getName()
			<< ": drawing slice " << slice->getId()
			<< " in section " << section << std::endl;

	double z = _zScale*section;

	glCheck(glColor4f(red, green, blue, 1.0));

	_textures.get(slice->getId())->bind();

	glBegin(GL_QUADS);

	const util::rect<double>& bb = slice->getComponent()->getBoundingBox();

	if (_leftSide) {

		glTexCoord2d(1.0, 0.0); glNormal3d(0, 0, -1); glVertex3d(bb.maxX, bb.minY, z);
		glTexCoord2d(1.0, 1.0); glNormal3d(0, 0, -1); glVertex3d(bb.maxX, bb.maxY, z);
		glTexCoord2d(0.0, 1.0); glNormal3d(0, 0, -1); glVertex3d(bb.minX, bb.maxY, z);
		glTexCoord2d(0.0, 0.0); glNormal3d(0, 0, -1); glVertex3d(bb.minX, bb.minY, z);
	}

	if (_rightSide) {

		glTexCoord2d(0.0, 0.0); glNormal3d(0, 0, 1); glVertex3d(bb.minX, bb.minY, z);
		glTexCoord2d(0.0, 1.0); glNormal3d(0, 0, 1); glVertex3d(bb.minX, bb.maxY, z);
		glTexCoord2d(1.0, 1.0); glNormal3d(0, 0, 1); glVertex3d(bb.maxX, bb.maxY, z);
		glTexCoord2d(1.0, 0.0); glNormal3d(0, 0, 1); glVertex3d(bb.maxX, bb.minY, z);
	}

	glCheck(glEnd());

	LOG_ALL(segmentspainterlog) << getName() << ": done." << std::endl;

	if (_size == util::rect<double>(0, 0, 0, 0))

		_size = bb;

	else {

		_size.minX = std::min(_size.minX, bb.minX);
		_size.minY = std::min(_size.minY, bb.minY);
		_size.maxX = std::max(_size.maxX, bb.maxX);
		_size.maxY = std::max(_size.maxY, bb.maxY);
	}
}
