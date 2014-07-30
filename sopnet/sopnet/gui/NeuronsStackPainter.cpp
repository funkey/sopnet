#include <boost/lexical_cast.hpp>

#include <gui/TextPainter.h>
#include "NeuronsStackPainter.h"

static logger::LogChannel neuronsstackpainterlog("neuronsstackpainterlog", "[NeuronsStackPainter] ");

NeuronsStackPainter::NeuronsStackPainter() :
	_section(0),
	_showSingleNeuron(false),
	_showCompleteNeurons(true),
	_currentNeuron(0),
	_showEnds(true),
	_showContinuations(true),
	_showBranches(true),
	_showSliceIds(false),
	_alpha(0.8),
	_zScale(15) {}

void
NeuronsStackPainter::setNeurons(boost::shared_ptr<SegmentTrees> neurons) {

	// aquire a read lock on the neurons
	boost::shared_lock<boost::shared_mutex> lockNeurons(neurons->getMutex());

	LOG_DEBUG(neuronsstackpainterlog) << "got new neurons" << std::endl;

	_neurons = neurons;

	assignColors();

	loadTextures();

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
	_showCompleteNeurons = true;
}

void
NeuronsStackPainter::showCompleteNeurons(bool show) {

	_showCompleteNeurons = show;
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

		LOG_ALL(neuronsstackpainterlog) << "assigning color to neuron " << i << std::endl;

		boost::shared_ptr<SegmentTree> neuron = (*_neurons)[i];

		double r;
		double g;
		double b;

		if (neuron->getContinuations().size() < 1) {

			hsvToRgb(100, 0.5, 0.5, r, g, b);

		} else {

			// get a hash for the neuron
			util::rect<int> bb = neuron->getContinuations()[0]->getSlices()[0]->getComponent()->getBoundingBox();

			int sum = bb.minX + bb.minY;
			srand(sum);

			LOG_ALL(neuronsstackpainterlog) << "random seed for this neuron is " << sum << std::endl;

			// draw a random color
			double h = (double)rand()/RAND_MAX*360.0;

			LOG_ALL(neuronsstackpainterlog) << "hue is " << h << std::endl;

			hsvToRgb(h, 1.0, 1.0, r, g, b);
		}

		_colors[i][0] = r;
		_colors[i][1] = g;
		_colors[i][2] = b;
	 }

	LOG_DEBUG(neuronsstackpainterlog) << "done" << std::endl;
}

void
NeuronsStackPainter::hsvToRgb(double h, double s, double v, double& r, double& g, double& b) {

	if(s < 0) s = 0;
	if(s > 1) s = 1;
	if(v < 0) v = 0;
	if(v > 1) v = 1;

	if(s == 0) {
		r = v;
		g = v;
		b = v;
	}

	h = h - floorf(h/360.0); // want h to be in 0..1

	unsigned int i = h*6;
	double f = (h*6) - i;
	double p = v*(1.0f - s); 
	double q = v*(1.0f - s*f);
	double t = v*(1.0f - s*(1.0f-f));
	switch(i%6) {
	case 0:
		r = v;
		g = t;
		b = p;
		return;
	case 1:
		r = q;
		g = v;
		b = p;
		return;
	case 2:
		r = p;
		g = v;
		b = t;
		return;
	case 3:
		r = p;
		g = q;
		b = v;
		return;
	case 4:
		r = t;
		g = p;
		b = v;
		return;
	case 5:
		r = v;
		g = p;
		b = q;
		return;
	}
}

void
NeuronsStackPainter::loadTextures() {

	_textures.clear();

	foreach (boost::shared_ptr<SegmentTree> neuron, *_neurons) {

		// aquire a read lock on the neuron
		boost::shared_lock<boost::shared_mutex> lockNeuron(neuron->getMutex());

		foreach (boost::shared_ptr<EndSegment> end, neuron->getEnds())
			_textures.load(*end->getSlice());

		foreach (boost::shared_ptr<ContinuationSegment> continuation, neuron->getContinuations()) {

			_textures.load(*continuation->getSourceSlice());
			_textures.load(*continuation->getTargetSlice());
		}

		foreach (boost::shared_ptr<BranchSegment> branch, neuron->getBranches()) {

			_textures.load(*branch->getSourceSlice());
			_textures.load(*branch->getTargetSlice1());
			_textures.load(*branch->getTargetSlice2());
		}
	}
}

void
NeuronsStackPainter::setCurrentSection(unsigned int section) {

	_section = std::min(section, (unsigned int)std::max((int)0, (int)_neurons->getNumSections() - 1));

	// get the size of the painter
	util::rect<double> size(0, 0, 0, 0);

	foreach (boost::shared_ptr<SegmentTree> neuron, *_neurons) {

		foreach (boost::shared_ptr<EndSegment> end, neuron->getEnds())
			size = sizeAddSlice(size, *end->getSlice());

		foreach (boost::shared_ptr<ContinuationSegment> continuation, neuron->getContinuations()) {

			size = sizeAddSlice(size, *continuation->getSourceSlice());
			size = sizeAddSlice(size, *continuation->getTargetSlice());
		}

		foreach (boost::shared_ptr<BranchSegment> branch, neuron->getBranches()) {

			size = sizeAddSlice(size, *branch->getSourceSlice());
			size = sizeAddSlice(size, *branch->getTargetSlice1());
			size = sizeAddSlice(size, *branch->getTargetSlice2());
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

bool
NeuronsStackPainter::draw(
		const util::rect<double>& roi,
		const util::point<double>& resolution) {

	// aquire a read lock
	boost::shared_lock<boost::shared_mutex> lockMe(getMutex());
	boost::shared_lock<boost::shared_mutex> lockNeurons(_neurons->getMutex());

	if (_neurons->size() == 0)
		return false;

	LOG_ALL(neuronsstackpainterlog) << "redrawing section " << _section << std::endl;

	// from previous section

	if (_showSingleNeuron) {

		_currentNeuron = std::min(_currentNeuron, _neurons->size() - 1);

		drawNeuron(*(*(_neurons->begin() + _currentNeuron)), _currentNeuron, roi, resolution);

	} else {

		unsigned int neuronNum = 0;
		foreach (boost::shared_ptr<SegmentTree> neuron, *_neurons) {

			// read lock on neuron
			boost::shared_lock<boost::shared_mutex> lockNeuron(neuron->getMutex());

			drawNeuron(*neuron, neuronNum, roi, resolution);
			neuronNum++;
		}
	}

	return false;
}

void
NeuronsStackPainter::drawNeuron(
		SegmentTree& neuron,
		unsigned int neuronNum,
		const util::rect<double>&  roi,
		const util::point<double>& resolution) {

	if (neuron.getContinuations().size() >= 1 && !_showCompleteNeurons)
		return;

	double red   = _colors[neuronNum][0];
	double green = _colors[neuronNum][1];
	double blue  = _colors[neuronNum][2];

	// we want to show the slices of the current section and the links to slices 
	// in the previous and next section

	// clear memory of drawn slices
	_drawnSlices = std::set<unsigned int>();

	if (_showEnds) {

		// for all ends in the previous inter-section interval
		foreach (boost::shared_ptr<EndSegment> end, neuron.getEnds(_section)) {

			// is their slice in our section?
			if (end->getDirection() == Left) {

				// draw the slice
				drawSlice(*end->getSlice(), red, green, blue, _alpha, roi, resolution);

				// draw the end-link
				drawEnd(end->getSlice()->getComponent()->getCenter(), Left, roi, resolution);
			}
		}

		// for all ends in the next inter-section interval
		foreach (boost::shared_ptr<EndSegment> end, neuron.getEnds(_section + 1)) {

			// is their slice in our section?
			if (end->getDirection() == Right) {

				// draw the slice, it it wasn't drawn by previous segment to the
				// right
				if (_section == 0)
					drawSlice(*end->getSlice(), red, green, blue, _alpha, roi, resolution);

				// draw the end-link
				drawEnd(end->getSlice()->getComponent()->getCenter(), Right, roi, resolution);
			}
		}
	}

	if (_showContinuations) {

		// for all continuations in the previous inter-section interval
		foreach (boost::shared_ptr<ContinuationSegment> continuation, neuron.getContinuations(_section)) {

			if (continuation->getDirection() == Left) {

				drawSlice(
						*continuation->getSourceSlice(),
						red, green, blue,
						_alpha,
						roi, resolution);

				drawContinuation(
						continuation->getSourceSlice()->getComponent()->getCenter(),
						continuation->getTargetSlice()->getComponent()->getCenter(),
						Left,
						roi, resolution);
			} else {

				drawSlice(
						*continuation->getTargetSlice(),
						red, green, blue,
						_alpha,
						roi, resolution);

				drawContinuation(
						continuation->getTargetSlice()->getComponent()->getCenter(),
						continuation->getSourceSlice()->getComponent()->getCenter(),
						Left,
						roi, resolution);
			}
		}

		// for all continuations in the next inter-section interval
		foreach (boost::shared_ptr<ContinuationSegment> continuation, neuron.getContinuations(_section + 1)) {

			if (continuation->getDirection() == Left) {

				// draw the slice, it it wasn't drawn by previous segment to the
				// right
				if (_section == 0)
					drawSlice(
							*continuation->getTargetSlice(),
							red, green, blue,
							_alpha,
							roi, resolution);

				drawContinuation(
						continuation->getTargetSlice()->getComponent()->getCenter(),
						continuation->getSourceSlice()->getComponent()->getCenter(),
						Right,
						roi, resolution);
			} else {

				// draw the slice, it it wasn't drawn by previous segment to the
				// right
				if (_section == 0)
					drawSlice(
							*continuation->getSourceSlice(),
							red, green, blue,
							_alpha,
							roi, resolution);

				drawContinuation(
						continuation->getSourceSlice()->getComponent()->getCenter(),
						continuation->getTargetSlice()->getComponent()->getCenter(),
						Right,
						roi, resolution);
			}
		}
	}

	if (_showBranches) {

		// for all branches in the previous inter-section interval
		foreach (boost::shared_ptr<BranchSegment> branch, neuron.getBranches(_section)) {

			if (branch->getDirection() == Left) {

				drawSlice(
						*branch->getSourceSlice(),
						red, green, blue,
						_alpha,
						roi, resolution);

				drawBranch(
						branch->getSourceSlice()->getComponent()->getCenter(),
						branch->getTargetSlice1()->getComponent()->getCenter(),
						branch->getTargetSlice2()->getComponent()->getCenter(),
						Left,
						roi, resolution);

			} else {

				drawSlice(
						*branch->getTargetSlice1(),
						red, green, blue,
						_alpha,
						roi, resolution);

				drawSlice(
						*branch->getTargetSlice2(),
						red, green, blue,
						_alpha,
						roi, resolution);

				drawMerge(
						branch->getTargetSlice1()->getComponent()->getCenter(),
						branch->getTargetSlice2()->getComponent()->getCenter(),
						branch->getSourceSlice()->getComponent()->getCenter(),
						Left,
						roi, resolution);
			}
		}

		// for all branches in the next inter-section interval
		foreach (boost::shared_ptr<BranchSegment> branch, neuron.getBranches(_section + 1)) {

			if (branch->getDirection() == Left) {

				// draw the slices, it they weren't drawn by previous segment to 
				// the right
				if (_section == 0) {

					drawSlice(
							*branch->getTargetSlice1(),
							red, green, blue,
							_alpha,
							roi, resolution);
					drawSlice(
							*branch->getTargetSlice2(),
							red, green, blue,
							_alpha,
							roi, resolution);
				}

				drawMerge(
						branch->getTargetSlice1()->getComponent()->getCenter(),
						branch->getTargetSlice2()->getComponent()->getCenter(),
						branch->getSourceSlice()->getComponent()->getCenter(),
						Right,
						roi, resolution);

			} else {

				// draw the slice, it it wasn't drawn by previous segment to the
				// right
				if (_section == 0)
					drawSlice(
							*branch->getSourceSlice(),
							red, green, blue,
							_alpha,
							roi, resolution);

				drawBranch(
						branch->getSourceSlice()->getComponent()->getCenter(),
						branch->getTargetSlice1()->getComponent()->getCenter(),
						branch->getTargetSlice2()->getComponent()->getCenter(),
						Right,
						roi, resolution);
			}
		}
	}

	glCheck(glDisable(GL_BLEND));
	glCheck(glDisable(GL_CULL_FACE));
}

void
NeuronsStackPainter::drawSlice(
		const Slice& slice,
		double red, double green, double blue,
		double alpha,
		const util::rect<double>&  roi,
		const util::point<double>& resolution) {

	if (_drawnSlices.count(slice.getId()))
		return;

	_drawnSlices.insert(slice.getId());

	glCheck(glColor4f(red, green, blue, alpha));

	glCheck(glEnable(GL_TEXTURE_2D));

	glCheck(glEnable(GL_BLEND));
	glCheck(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

	glCheck(glEnable(GL_CULL_FACE));
	glCheck(glEnable(GL_LIGHTING));
	glCheck(glEnable(GL_LIGHT0));
	glCheck(glEnable(GL_COLOR_MATERIAL));

	try {

		_textures.get(slice.getId())->bind();

	} catch (SliceTextures::MissingTexture& e) {

		loadTextures();
		_textures.get(slice.getId())->bind();
	}

	glBegin(GL_QUADS);

	util::rect<double> bb = slice.getComponent()->getBoundingBox();
	bb.maxX += 2;
	bb.maxY += 2;

	// right side
	glTexCoord2d(0.0, 0.0); glNormal3d(0, 0, 1); glVertex3d(bb.minX, bb.minY, 0);
	glTexCoord2d(0.0, 1.0); glNormal3d(0, 0, 1); glVertex3d(bb.minX, bb.maxY, 0);
	glTexCoord2d(1.0, 1.0); glNormal3d(0, 0, 1); glVertex3d(bb.maxX, bb.maxY, 0);
	glTexCoord2d(1.0, 0.0); glNormal3d(0, 0, 1); glVertex3d(bb.maxX, bb.minY, 0);

	glCheck(glEnd());

	if (_showSliceIds) {

		gui::TextPainter idPainter(boost::lexical_cast<std::string>(slice.getId()));
		idPainter.setTextSize(10.0);
		idPainter.setTextColor(1.0 - red, 1.0 - green, 1.0 - blue);

		double x = slice.getComponent()->getCenter().x;
		double y = slice.getComponent()->getCenter().y;

		glPushMatrix();
		glTranslatef(x, y, 0);
		idPainter.draw(roi - util::point<double>(x, y), resolution);
		glPopMatrix();
	}
}

void
NeuronsStackPainter::drawEnd(
		const util::point<double>& center,
		Direction direction,
		const util::rect<double>&,
		const util::point<double>&) {

	if (direction == Left)
		setPrevColor();
	else
		setNextColor();

	double size = 7;

	glCheck(glDisable(GL_TEXTURE_2D));
	glCheck(glEnable(GL_LINE_SMOOTH));
	glCheck(glLineWidth(5.0));

	glBegin(GL_QUADS);
	glVertex3d(center.x - size, center.y - size, 0);
	glVertex3d(center.x - size, center.y + size, 0);
	glVertex3d(center.x + size, center.y + size, 0);
	glVertex3d(center.x + size, center.y - size, 0);
	glEnd();
}

void
NeuronsStackPainter::drawContinuation(
		const util::point<double>& source,
		const util::point<double>& target,
		Direction direction,
		const util::rect<double>&,
		const util::point<double>&) {

	if (direction == Left)
		setPrevColor();
	else
		setNextColor();

	glCheck(glDisable(GL_TEXTURE_2D));
	glCheck(glEnable(GL_LINE_SMOOTH));
	glCheck(glLineWidth(5.0));

	glBegin(GL_LINES);
	glVertex3d(source.x, source.y, 0);
	glVertex3d(target.x, target.y, 0);
	glEnd();
}

void
NeuronsStackPainter::drawBranch(
		const util::point<double>& source,
		const util::point<double>& target1,
		const util::point<double>& target2,
		Direction direction,
		const util::rect<double>&,
		const util::point<double>&) {

	if (direction == Left)
		setPrevColor();
	else
		setNextColor();

	glCheck(glDisable(GL_TEXTURE_2D));

	glBegin(GL_LINES);
	glVertex3d(source.x, source.y, 0);
	glVertex3d(target1.x, target1.y, 0);
	glEnd();
	glBegin(GL_LINES);
	glVertex3d(source.x, source.y, 0);
	glVertex3d(target2.x, target2.y, 0);
	glEnd();
}

void
NeuronsStackPainter::drawMerge(
		const util::point<double>& source1,
		const util::point<double>& source2,
		const util::point<double>& target,
		Direction direction,
		const util::rect<double>&,
		const util::point<double>&) {

	if (direction == Left)
		setPrevColor();
	else
		setNextColor();

	glCheck(glDisable(GL_TEXTURE_2D));
	glCheck(glEnable(GL_LINE_SMOOTH));
	glCheck(glLineWidth(5.0));

	glBegin(GL_LINES);
	glVertex3d(source1.x, source1.y, 0);
	glVertex3d(target.x, target.y, 0);
	glEnd();
	glBegin(GL_LINES);
	glVertex3d(source2.x, source2.y, 0);
	glVertex3d(target.x, target.y, 0);
	glEnd();
}

void
NeuronsStackPainter::setNextColor() {

	glColor4f(0.9, 0.9, 1.0, _alpha);
}

void
NeuronsStackPainter::setPrevColor() {

	glColor4f(0.1, 0.1, 0.0, _alpha);
}
