#include <boost/array.hpp>

#include <gui/OpenGl.h>
#include <imageprocessing/ConnectedComponent.h>
#include <sopnet/segments/EndSegment.h>
#include <sopnet/segments/ContinuationSegment.h>
#include <sopnet/segments/BranchSegment.h>
#include <util/foreach.h>
#include "SegmentsPainter.h"

logger::LogChannel segmentspainterlog("segmentspainterlog", "[SegmentsPainter] ");

SegmentsPainter::SegmentsPainter() :
	_zScale(8) {}

SegmentsPainter::~SegmentsPainter() {

	deleteTextures();
}

void
SegmentsPainter::setSegments(boost::shared_ptr<Segments> segments) {

	_segments = segments;

	loadTextures();

	util::rect<double> size(0, 0, 0, 0);

	updateRecording();

	LOG_ALL(segmentspainterlog) << "size is " << _size << std::endl;

	setSize(_size);
}

void
SegmentsPainter::loadTextures() {

	LOG_DEBUG(segmentspainterlog) << "loading textures..." << std::endl;

	deleteTextures();

	foreach (boost::shared_ptr<EndSegment> segment, _segments->getEnds())
		loadTextures(*segment);

	foreach (boost::shared_ptr<ContinuationSegment> segment, _segments->getContinuations())
		loadTextures(*segment);

	foreach (boost::shared_ptr<BranchSegment> segment, _segments->getBranches())
		loadTextures(*segment);

	LOG_DEBUG(segmentspainterlog) << "all textures loaded..." << std::endl;
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

	if (_textures.count(slice.getId()))
		return;

	// create image data
	const util::rect<double> bb = slice.getComponent()->getBoundingBox();

	util::point<unsigned int> size(bb.maxX - bb.minX + 2, bb.maxY - bb.minY + 2);
	util::point<unsigned int> offset(bb.minX, bb.minY);

	// rgba data
	std::vector<boost::array<float, 4> > pixels;

	// fill with opaque value

	boost::array<float, 4> opaque;
	opaque[0] = 0.0;
	opaque[1] = 0.0;
	opaque[2] = 0.0;
	opaque[3] = 0.5;
	pixels.resize(size.x*size.y, opaque);

	foreach (const util::point<unsigned int>& pixel, slice.getComponent()->getPixels()) {

		unsigned int index = (pixel.x - offset.x) + (pixel.y - offset.y)*size.x;

		pixels[index][0] = 1.0;
		pixels[index][1] = 1.0;
		pixels[index][2] = 1.0;
		pixels[index][3] = 1.0;
	}

	gui::Texture* texture = new gui::Texture(size.x, size.y, GL_RGBA);

	texture->loadData(pixels.begin());

	_textures[slice.getId()] = texture;
}

void
SegmentsPainter::updateRecording() {

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

	// draw from front to back, facing to the right
	_leftSide  = false;
	_rightSide = true;
	for (int i = 0; i < _segments->getNumInterSectionIntervals(); i++) {

		foreach (boost::shared_ptr<EndSegment> segment, _segments->getEnds(i))
			draw(*segment);

		foreach (boost::shared_ptr<ContinuationSegment> segment, _segments->getContinuations(i))
			draw(*segment);

		foreach (boost::shared_ptr<BranchSegment> segment, _segments->getBranches(i))
			draw(*segment);
	}

	// draw from back to front, facing to the left
	_leftSide  = true;
	_rightSide = false;
	for (int i = _segments->getNumInterSectionIntervals() - 1; i >= 0; i--) {

		foreach (boost::shared_ptr<EndSegment> segment, _segments->getEnds(i))
			draw(*segment);

		foreach (boost::shared_ptr<ContinuationSegment> segment, _segments->getContinuations(i))
			draw(*segment);

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

	drawSlice(end.getSlice(), 1.0, 0.0, 0.0);

	glCheck(glDisable(GL_TEXTURE_2D));
}

void
SegmentsPainter::draw(const ContinuationSegment& continuation) {

	glCheck(glEnable(GL_TEXTURE_2D));

	drawSlice(continuation.getSourceSlice(), 0.0, 1.0, 0.0);
	drawSlice(continuation.getTargetSlice(), 0.0, 1.0, 0.0);

	util::point<double> center1 = continuation.getSourceSlice()->getComponent()->getCenter();
	util::point<double> center2 = continuation.getTargetSlice()->getComponent()->getCenter();

	unsigned int section1 = continuation.getSourceSlice()->getSection();
	unsigned int section2 = continuation.getTargetSlice()->getSection();

	glCheck(glDisable(GL_TEXTURE_2D));

	glCheck(glBegin(GL_LINES));

	glCheck(glVertex3f(center1.x, center1.y, section1*_zScale));
	glCheck(glVertex3f(center2.x, center2.y, section2*_zScale));

	glCheck(glEnd());
}

void
SegmentsPainter::draw(const BranchSegment& branch) {

	glCheck(glEnable(GL_TEXTURE_2D));

	drawSlice(branch.getSourceSlice(), 0.0, 0.0, 1.0);
	drawSlice(branch.getTargetSlice1(), 0.0, 0.0, 1.0);
	drawSlice(branch.getTargetSlice2(), 0.0, 0.0, 1.0);

	util::point<double> center1 = branch.getSourceSlice()->getComponent()->getCenter();
	util::point<double> center2 = branch.getTargetSlice1()->getComponent()->getCenter();
	util::point<double> center3 = branch.getTargetSlice2()->getComponent()->getCenter();

	unsigned int section1 = branch.getSourceSlice()->getSection();
	unsigned int section2 = branch.getTargetSlice1()->getSection();
	unsigned int section3 = branch.getTargetSlice2()->getSection();

	glCheck(glDisable(GL_TEXTURE_2D));

	glCheck(glBegin(GL_LINES));

	glCheck(glVertex3f(center1.x, center1.y, section1*_zScale));
	glCheck(glVertex3f(center2.x, center2.y, section2*_zScale));

	glCheck(glEnd());

	glCheck(glBegin(GL_LINES));

	glCheck(glVertex3f(center1.x, center1.y, section1*_zScale));
	glCheck(glVertex3f(center3.x, center3.y, section3*_zScale));

	glCheck(glEnd());
}

void
SegmentsPainter::drawSlice(
		boost::shared_ptr<Slice> slice,
		double red, double green, double blue) {

	double section = slice->getSection();

	LOG_ALL(segmentspainterlog)
			<< "drawing slice " << slice->getId()
			<< " in section " << section << std::endl;

	double z = _zScale*section;

	glCheck(glColor4f(red, green, blue, 1.0));

	_textures[slice->getId()]->bind();

	glCheck(glBegin(GL_QUADS));

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

	LOG_ALL(segmentspainterlog) << "done." << std::endl;

	if (_size == util::rect<double>(0, 0, 0, 0))

		_size = bb;

	else {

		_size.minX = std::min(_size.minX, bb.minX);
		_size.minY = std::min(_size.minY, bb.minY);
		_size.maxX = std::max(_size.maxX, bb.maxX);
		_size.maxY = std::max(_size.maxY, bb.maxY);
	}
}

void
SegmentsPainter::deleteTextures() {

	unsigned int id;
	gui::Texture* texture;
	foreach (boost::tie(id, texture), _textures)
		if (texture)
			delete texture;

	_textures.clear();
}
