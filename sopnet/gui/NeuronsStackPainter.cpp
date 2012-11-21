#include <boost/lexical_cast.hpp>

#include <gui/TextPainter.h>
#include "NeuronsStackPainter.h"

static logger::LogChannel neuronsstackpainterlog("neuronsstackpainterlog", "[NeuronsStackPainter] ");

NeuronsStackPainter::NeuronsStackPainter() :
	_section(0),
	_showSingleNeuron(false),
	_currentNeuron(0),
	_showEnds(true),
	_showContinuations(true),
	_showBranches(true),
	_showSliceIds(false),
	_zScale(15) {}

void
NeuronsStackPainter::setNeurons(boost::shared_ptr<Neurons> neurons) {

	LOG_DEBUG(neuronsstackpainterlog) << "got new neurons" << std::endl;

	_neurons = neurons;

	assignColors();

	_textures.clear();

	setCurrentSection(_section);
}

void
NeuronsStackPainter::showNeuron(unsigned int neuron) {

	if (neuron >= _neurons->size())
		return;

	_showSingleNeuron = true;
	_currentNeuron = neuron;
}

void
NeuronsStackPainter::showAllNeurons() {

	_showSingleNeuron = false;
}

void
NeuronsStackPainter::showEnds(bool show) {

	LOG_DEBUG(neuronsstackpainterlog) << "showing ends: " << show << std::endl;

	_showEnds = show;
}

void
NeuronsStackPainter::showContinuations(bool show) {

	LOG_DEBUG(neuronsstackpainterlog) << "showing continuations: " << show << std::endl;

	_showContinuations = show;
}

void
NeuronsStackPainter::showBranches(bool show) {

	LOG_DEBUG(neuronsstackpainterlog) << "showing branches: " << show << std::endl;

	_showBranches = show;
}

void
NeuronsStackPainter::showSliceIds(bool show) {

	_showSliceIds = show;
}

void
NeuronsStackPainter::assignColors() {

	LOG_DEBUG(neuronsstackpainterlog) << "assigning colors" << std::endl;

	_colors.clear();

	// assign a color to each neuron

	for (unsigned int i = 0; i < _neurons->size(); i++) {

		// draw a random color
		double r = (double)rand()/RAND_MAX;
		double g = (double)rand()/RAND_MAX;
		double b = (double)rand()/RAND_MAX;

		_colors[i][0] = r;
		_colors[i][1] = g;
		_colors[i][2] = b;
	 }

	LOG_DEBUG(neuronsstackpainterlog) << "done" << std::endl;
}

void
NeuronsStackPainter::setCurrentSection(unsigned int section) {

	_section = std::min(section, _neurons->getNumSections() - 1);

	// get the size of the painter and load missing textures
	util::rect<double> size(0, 0, 0, 0);

	foreach (boost::shared_ptr<Neuron> neuron, *_neurons) {

		foreach (boost::shared_ptr<EndSegment> end, neuron->getEnds()) {

			size = sizeAddSlice(size, *end->getSlice());

			_textures.load(*end->getSlice());
		}

		foreach (boost::shared_ptr<ContinuationSegment> continuation, neuron->getContinuations()) {

			size = sizeAddSlice(size, *continuation->getSourceSlice());
			size = sizeAddSlice(size, *continuation->getTargetSlice());

			_textures.load(*continuation->getSourceSlice());
			_textures.load(*continuation->getTargetSlice());
		}

		foreach (boost::shared_ptr<BranchSegment> branch, neuron->getBranches()) {

			size = sizeAddSlice(size, *branch->getSourceSlice());
			size = sizeAddSlice(size, *branch->getTargetSlice1());
			size = sizeAddSlice(size, *branch->getTargetSlice2());

			_textures.load(*branch->getSourceSlice());
			_textures.load(*branch->getTargetSlice1());
			_textures.load(*branch->getTargetSlice2());
		}
	}

	setSize(size);

	LOG_DEBUG(neuronsstackpainterlog) << "current section set to " << _section << std::endl;
	LOG_DEBUG(neuronsstackpainterlog) << "current size set to " << size << std::endl;
}

util::rect<double>
NeuronsStackPainter::sizeAddSlice(const util::rect<double>& currentSize, const Slice& slice) {

	if (currentSize.width() == 0 && currentSize.height() == 0)
		return slice.getComponent()->getBoundingBox();

	util::rect<double> size;

	size.minX = std::min(currentSize.minX, (double)slice.getComponent()->getBoundingBox().minX);
	size.minY = std::min(currentSize.minY, (double)slice.getComponent()->getBoundingBox().minY);
	size.maxX = std::max(currentSize.maxX, (double)slice.getComponent()->getBoundingBox().maxX);
	size.maxY = std::max(currentSize.maxY, (double)slice.getComponent()->getBoundingBox().maxY);

	return size;
}

void
NeuronsStackPainter::draw(
		const util::rect<double>& roi,
		const util::point<double>& resolution) {

	LOG_ALL(neuronsstackpainterlog) << "redrawing section " << _section << std::endl;

	// from previous section

	if (_showSingleNeuron) {

		drawNeuron(*(*(_neurons->begin() + _currentNeuron)), _currentNeuron, roi, resolution);

	} else {

		unsigned int neuronNum = 0;
		foreach (boost::shared_ptr<Neuron> neuron, *_neurons) {

			drawNeuron(*neuron, neuronNum, roi, resolution);
			neuronNum++;
		}
	}
}

void
NeuronsStackPainter::drawNeuron(
		Neuron& neuron,
		unsigned int neuronNum,
		const util::rect<double>&  roi,
		const util::point<double>& resolution) {

	double red   = _colors[neuronNum][0];
	double green = _colors[neuronNum][1];
	double blue  = _colors[neuronNum][2];

	if (_showEnds) {

		foreach (boost::shared_ptr<EndSegment> end, neuron.getEnds(_section)) {

			if (end->getDirection() == Left) {

				LOG_ALL(neuronsstackpainterlog) << "drawing an end..." << std::endl;

				drawSlice(*end->getSlice(), 0.0, red, green, blue, 0.75, roi, resolution);

				LOG_ALL(neuronsstackpainterlog) << "done" << std::endl;
			}
		}
	}

	if (_showContinuations) {

		foreach (boost::shared_ptr<ContinuationSegment> continuation, neuron.getContinuations(_section)) {

			LOG_ALL(neuronsstackpainterlog) << "drawing a continuation..." << std::endl;

			drawSlice(
					*continuation->getSourceSlice(),
					(continuation->getDirection() == Right ? -_zScale : 0.0),
					red, green, blue,
					(continuation->getDirection() == Right ? 0.25 : 0.75),
					roi, resolution);

			drawSlice(
					*continuation->getTargetSlice(),
					(continuation->getDirection() == Left  ? -_zScale : 0.0),
					red, green, blue,
					(continuation->getDirection() == Left  ? 0.25 : 0.75),
					roi, resolution);

			LOG_ALL(neuronsstackpainterlog) << "done" << std::endl;
		}
	}

	if (_showBranches) {

		foreach (boost::shared_ptr<BranchSegment> branch, neuron.getBranches(_section)) {

			LOG_ALL(neuronsstackpainterlog) << "drawing a branch..." << std::endl;

			drawSlice(
					*branch->getSourceSlice(),
					(branch->getDirection() == Right ? -_zScale : 0.0),
					red, green, blue,
					(branch->getDirection() == Right ? 0.25 : 0.75),
					roi, resolution);

			drawSlice(
					*branch->getTargetSlice1(),
					(branch->getDirection() == Left  ? -_zScale : 0.0),
					red, green, blue,
					(branch->getDirection() == Left ? 0.25 : 0.75),
					roi, resolution);

			drawSlice(
					*branch->getTargetSlice2(),
					(branch->getDirection() == Left  ? -_zScale : 0.0),
					red, green, blue,
					(branch->getDirection() == Left ? 0.25 : 0.75),
					roi, resolution);

			LOG_ALL(neuronsstackpainterlog) << "done" << std::endl;
		}
	}

	// to next section

	if (_showEnds) {

		foreach (boost::shared_ptr<EndSegment> end, neuron.getEnds(_section + 1)) {

			if (end->getDirection() == Right) {

				LOG_ALL(neuronsstackpainterlog) << "drawing an end..." << std::endl;

				drawSlice(*end->getSlice(), 0.0, red, green, blue, 0.75, roi, resolution);

				LOG_ALL(neuronsstackpainterlog) << "done" << std::endl;
			}
		}
	}

	if (_showContinuations) {

		foreach (boost::shared_ptr<ContinuationSegment> continuation, neuron.getContinuations(_section + 1)) {

			LOG_ALL(neuronsstackpainterlog) << "drawing a continuation..." << std::endl;

			drawSlice(
					*continuation->getSourceSlice(),
					(continuation->getDirection() == Left ? _zScale : 0.0),
					red, green, blue,
					(continuation->getDirection() == Left ? 0.25 : 0.75),
					roi, resolution);

			drawSlice(
					*continuation->getTargetSlice(),
					(continuation->getDirection() == Right ? _zScale : 0.0),
					red, green, blue,
					(continuation->getDirection() == Right ? 0.25 : 0.75),
					roi, resolution);

			LOG_ALL(neuronsstackpainterlog) << "done" << std::endl;
		}
	}

	if (_showBranches) {

		foreach (boost::shared_ptr<BranchSegment> branch, neuron.getBranches(_section + 1)) {

			LOG_ALL(neuronsstackpainterlog) << "drawing a branch..." << std::endl;

			drawSlice(
					*branch->getSourceSlice(),
					(branch->getDirection() == Left  ? _zScale : 0.0),
					red, green, blue,
					(branch->getDirection() == Left ? 0.25 : 0.75),
					roi, resolution);

			drawSlice(
					*branch->getTargetSlice1(),
					(branch->getDirection() == Right ? _zScale : 0.0),
					red, green, blue,
					(branch->getDirection() == Right ? 0.25 : 0.75),
					roi, resolution);

			drawSlice(
					*branch->getTargetSlice2(),
					(branch->getDirection() == Right ? _zScale : 0.0),
					red, green, blue,
					(branch->getDirection() == Right ? 0.25 : 0.75),
					roi, resolution);

			LOG_ALL(neuronsstackpainterlog) << "done" << std::endl;
		}
	}
}

void
NeuronsStackPainter::drawSlice(
		const Slice& slice,
		double z,
		double red, double green, double blue,
		double alpha,
		const util::rect<double>&  roi,
		const util::point<double>& resolution) {

	// set up lighting
	GLfloat ambient[4] = { 0, 0, 0, 1 };
	glCheck(glLightfv(GL_LIGHT0, GL_AMBIENT, ambient));
	GLfloat diffuse[4] = { 0.1, 0.1, 0.1, 1 };
	glCheck(glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse));
	GLfloat specular[4] = { 0.1, 0.1, 0.1, 1 };
	glCheck(glLightfv(GL_LIGHT0, GL_SPECULAR, specular));
	glCheck(glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular)); GLfloat emission[4] = { 0, 0, 0, 1 };
	glCheck(glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emission));

	// enable alpha blending
	glCheck(glEnable(GL_BLEND));
	glCheck(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

	glCheck(glEnable(GL_CULL_FACE));
	glCheck(glEnable(GL_LIGHTING));
	glCheck(glEnable(GL_LIGHT0));
	glCheck(glEnable(GL_COLOR_MATERIAL));

	glCheck(glColor4f(red, green, blue, alpha));

	_textures.get(slice.getId())->bind();

	glBegin(GL_QUADS);

	const util::rect<double>& bb = slice.getComponent()->getBoundingBox();

	double offset = 0;

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

	glCheck(glDisable(GL_BLEND));
	glCheck(glDisable(GL_LIGHTING));
	glCheck(glDisable(GL_CULL_FACE));

	if (_showSliceIds) {

		gui::TextPainter idPainter(boost::lexical_cast<std::string>(slice.getId()));
		idPainter.setTextSize(10.0);
		idPainter.setTextColor(1.0 - red, 1.0 - green, 1.0 - blue);

		double x = slice.getComponent()->getCenter().x;
		double y = slice.getComponent()->getCenter().y + offset;

		glTranslatef(x, y, z);
		idPainter.draw(roi - util::point<double>(x, y), resolution);
		glTranslatef(-x, -y, -z);
	}
}

