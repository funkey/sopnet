#include "SegmentsPainter.h"
#include <imageprocessing/ConnectedComponent.h>
#include <sopnet/EndSegment.h>
#include <sopnet/ContinuationSegment.h>
#include <sopnet/BranchSegment.h>
#include <util/foreach.h>

logger::LogChannel segmentspainterlog("segmentspainterlog", "[SegmentsPainter] ");

SegmentsPainter::SegmentsPainter() :
	_zScale(8) {}

void
SegmentsPainter::setSegments(boost::shared_ptr<Segments> segments) {

	_segments = segments;

	util::rect<double> size(0, 0, 0, 0);

	updateRecording();

	LOG_ALL(segmentspainterlog) << "size is " << _size << std::endl;

	setSize(_size);
}

void
SegmentsPainter::updateRecording() {

	// make sure OpenGl operations are save
	gui::OpenGl::Guard guard;

	startRecording();

	GLfloat ambient[4] = { 0, 0, 0, 1 };
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
	GLfloat diffuse[4] = { 0.1, 0.1, 0.1, 1 };
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
	GLfloat specular[4] = { 0.1, 0.1, 0.1, 1 };
	glLightfv(GL_LIGHT0, GL_SPECULAR, specular);

	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
	GLfloat emission[4] = { 0, 0, 0, 1 };
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emission);

	// enable alpha blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// always draw the components
	glDisable(GL_CULL_FACE);

	glEnable(GL_DEPTH_TEST);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_COLOR_MATERIAL);
	glDisable(GL_TEXTURE_2D);

	foreach (boost::shared_ptr<EndSegment> segment, _segments->getEnds())
		draw(*segment);

	foreach (boost::shared_ptr<ContinuationSegment> segment, _segments->getContinuations())
		draw(*segment);

	foreach (boost::shared_ptr<BranchSegment> segment, _segments->getBranches())
		draw(*segment);

	glDisable(GL_BLEND);

	glDisable(GL_LIGHTING);

	stopRecording();
}

void
SegmentsPainter::draw(const EndSegment& end) {

	drawSlice(
			end.getSlice(),
			1.0, 0.0, 0.0,
			(end.getDirection() == Left  ? true : false),
			(end.getDirection() == Right ? true : false),
			true);
}

void
SegmentsPainter::draw(const ContinuationSegment& continuation) {

	drawSlice(
			continuation.getSourceSlice(),
			0.0, 1.0, 0.0,
			(continuation.getDirection() == Left ? true  : false),
			(continuation.getDirection() == Left ? false : true),
			true);
	drawSlice(
			continuation.getTargetSlice(),
			0.0, 1.0, 0.0,
			(continuation.getDirection() == Left ? false : true),
			(continuation.getDirection() == Left ? true  : false),
			true);

	util::point<double> center1 = continuation.getSourceSlice()->getComponent()->getCenter();
	util::point<double> center2 = continuation.getTargetSlice()->getComponent()->getCenter();

	unsigned int section1 = continuation.getSourceSlice()->getSection();
	unsigned int section2 = continuation.getTargetSlice()->getSection();

	glBegin(GL_LINES);

	glVertex3f(center1.x, center1.y, section1*_zScale);
	glVertex3f(center2.x, center2.y, section2*_zScale);

	glEnd();
}

void
SegmentsPainter::draw(const BranchSegment& branch) {

	drawSlice(
			branch.getSourceSlice(),
			0.0, 0.0, 1.0,
			(branch.getDirection() == Left ? true  : false),
			(branch.getDirection() == Left ? false : true),
			true);
	drawSlice(
			branch.getTargetSlice1(),
			0.0, 0.0, 1.0,
			(branch.getDirection() == Left ? false : true),
			(branch.getDirection() == Left ? true  : false),
			true);
	drawSlice(
			branch.getTargetSlice2(),
			0.0, 0.0, 1.0,
			(branch.getDirection() == Left ? false : true),
			(branch.getDirection() == Left ? true  : false),
			true);

	util::point<double> center1 = branch.getSourceSlice()->getComponent()->getCenter();
	util::point<double> center2 = branch.getTargetSlice1()->getComponent()->getCenter();
	util::point<double> center3 = branch.getTargetSlice2()->getComponent()->getCenter();

	unsigned int section1 = branch.getSourceSlice()->getSection();
	unsigned int section2 = branch.getTargetSlice1()->getSection();
	unsigned int section3 = branch.getTargetSlice2()->getSection();

	glBegin(GL_LINES);

	glVertex3f(center1.x, center1.y, section1*_zScale);
	glVertex3f(center2.x, center2.y, section2*_zScale);

	glEnd();

	glBegin(GL_LINES);

	glVertex3f(center1.x, center1.y, section1*_zScale);
	glVertex3f(center3.x, center3.y, section3*_zScale);

	glEnd();
}

void
SegmentsPainter::drawSlice(
		boost::shared_ptr<Slice> slice,
		double red, double green, double blue,
		bool leftSide, bool rightSide,
		bool surface) {

	boost::shared_ptr<ConnectedComponent> component = slice->getComponent();

	double section = slice->getSection();

	double minZ = _zScale*(section - (leftSide  ? 0.5 : 0));
	double maxZ = _zScale*(section + (rightSide ? 0.5 : 0));

	unsigned int width   = (unsigned int)(component->getBoundingBox().width()  + 1);
	unsigned int height  = (unsigned int)(component->getBoundingBox().height() + 1);
	unsigned int offsetX = (unsigned int)component->getBoundingBox().minX;
	unsigned int offsetY = (unsigned int)component->getBoundingBox().minY;

	std::vector<bool> pixels(width*height, false);

	unsigned int numPixels = 0;
	foreach (const util::point<unsigned int>& p, component->getPixels()) {

		unsigned int x = p.x - offsetX;
		unsigned int y = p.y - offsetY;

		if (x + y*width < width*height) {

			pixels[x + y*width] = true;
			numPixels++;

		} else {

			LOG_ERROR(segmentspainterlog) << "you are trying to access a pixel at " << p << " that is not within " << component->getBoundingBox() << std::endl;
		}
	}

	LOG_ALL(segmentspainterlog) << "drawing a slice with " << numPixels << " pixels..." << std::endl;

	glColor4f(red, green, blue, 1.0);

	glBegin(GL_QUADS);

	foreach (const util::point<unsigned int>& p, component->getPixels()) {

		// draw left or right side
		if (leftSide) {

			glNormal3d(0, 0, -1); glVertex3d(p.x,     p.y,     minZ);
			glNormal3d(0, 0, -1); glVertex3d(p.x + 1, p.y,     minZ);
			glNormal3d(0, 0, -1); glVertex3d(p.x + 1, p.y + 1, minZ);
			glNormal3d(0, 0, -1); glVertex3d(p.x,     p.y + 1, minZ);
		}

		if (rightSide) {

			glNormal3d(0, 0, 1);  glVertex3d(p.x,     p.y,     maxZ);
			glNormal3d(0, 0, 1);  glVertex3d(p.x + 1, p.y,     maxZ);
			glNormal3d(0, 0, 1);  glVertex3d(p.x + 1, p.y + 1, maxZ);
			glNormal3d(0, 0, 1);  glVertex3d(p.x,     p.y + 1, maxZ);
		}

		// only draw surface of visible pixels

		// for neighborhood testing with offset
		unsigned int x = p.x - offsetX;
		unsigned int y = p.y - offsetY;

		if (x > 0 && x < width - 1 && y > 0 && y < height - 1 &&
		    pixels[(x - 1) + (y - 1)*width] &&
		    pixels[(x - 1) + (y + 1)*width] &&
		    pixels[(x + 1) + (y + 1)*width] &&
		    pixels[(x + 1) + (y - 1)*width])
			continue;

		if (surface) {

			glNormal3d(0, -1, 0); glVertex3d(p.x,     p.y,     minZ);
			glNormal3d(0, -1, 0); glVertex3d(p.x + 1, p.y,     minZ);
			glNormal3d(0, -1, 0); glVertex3d(p.x + 1, p.y,     maxZ);
			glNormal3d(0, -1, 0); glVertex3d(p.x,     p.y,     maxZ);

			glNormal3d(-1, 0, 0); glVertex3d(p.x,     p.y,     minZ);
			glNormal3d(-1, 0, 0); glVertex3d(p.x,     p.y + 1, minZ);
			glNormal3d(-1, 0, 0); glVertex3d(p.x,     p.y + 1, maxZ);
			glNormal3d(-1, 0, 0); glVertex3d(p.x,     p.y,     maxZ);

			glNormal3d(0, 1, 0); glVertex3d(p.x + 1, p.y + 1, minZ);
			glNormal3d(0, 1, 0); glVertex3d(p.x,     p.y + 1, minZ);
			glNormal3d(0, 1, 0); glVertex3d(p.x,     p.y + 1, maxZ);
			glNormal3d(0, 1, 0); glVertex3d(p.x + 1, p.y + 1, maxZ);

			glNormal3d(1, 0, 0); glVertex3d(p.x + 1, p.y + 1, minZ);
			glNormal3d(1, 0, 0); glVertex3d(p.x + 1, p.y,     minZ);
			glNormal3d(1, 0, 0); glVertex3d(p.x + 1, p.y,     maxZ);
			glNormal3d(1, 0, 0); glVertex3d(p.x + 1, p.y + 1, maxZ);
		}
	}

	glEnd();

	LOG_ALL(segmentspainterlog) << "done." << std::endl;

	if (_size == util::rect<double>(0, 0, 0, 0))

		_size = component->getBoundingBox();

	else {

		_size.minX = std::min(_size.minX, component->getBoundingBox().minX);
		_size.minY = std::min(_size.minY, component->getBoundingBox().minY);
		_size.maxX = std::max(_size.maxX, component->getBoundingBox().maxX);
		_size.maxY = std::max(_size.maxY, component->getBoundingBox().maxY);
	}
}
