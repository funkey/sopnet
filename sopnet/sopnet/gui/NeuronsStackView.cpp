#include "NeuronsStackView.h"
#include <util/Logger.h>

static logger::LogChannel neuronsstackviewlog("neuronsstackviewlog", "[NeuronsStackView] ");

NeuronsStackView::NeuronsStackView() :
	_painter(new NeuronsStackPainter()),
	_section(0),
	_neuronsModified(true),
	_currentNeuronModified(false),
	_alpha(0.8),
	_selectedNeurons(new SegmentTrees()) {

	registerInput(_neurons, "neurons");
	registerInput(_currentNeuron, "current neuron", pipeline::Optional);
	registerOutput(_painter, "painter");
	registerOutput(_selection, "selection");

	_neurons.registerCallback(&NeuronsStackView::onNeuronsModified, this);
	_currentNeuron.registerCallback(&NeuronsStackView::onCurrentNeuronModified, this);

	_painter.registerSlot(_sizeChanged);
	_painter.registerSlot(_contentChanged);
	_painter.registerCallback(&NeuronsStackView::onKeyDown, this);
	_painter.registerCallback(&NeuronsStackView::onMouseDown, this);
	_painter->setSelection(_selectedNeurons);
	_painter->setAlpha(_alpha);
}

void
NeuronsStackView::onNeuronsModified(const pipeline::Modified&) {

	_neuronsModified = true;
}

void
NeuronsStackView::onCurrentNeuronModified(const pipeline::Modified&) {

	_currentNeuronModified = true;
}

void
NeuronsStackView::updateOutputs() {

	if (_neuronsModified) {

		_painter->setNeurons(_neurons);

		_neuronsModified = false;
	}

	util::rect<double> oldSize = _painter->getSize();

	if (_currentNeuronModified) {

		_painter->showNeuron(*_currentNeuron);

		_currentNeuronModified = false;
	}

	util::rect<double> newSize = _painter->getSize();

	if (oldSize == newSize) {

		LOG_ALL(neuronsstackviewlog) << "neurons size did not change -- sending ContentChanged" << std::endl;

		_contentChanged();

	} else {

		LOG_ALL(neuronsstackviewlog) << "neurons size did change -- sending SizeChanged" << std::endl;

		_sizeChanged();
	}

	if (!_selection)
		_selection = new SegmentTrees();

	*_selection = *_selectedNeurons;
}

void
NeuronsStackView::onKeyDown(gui::KeyDown& signal) {

	if (signal.processed)
		return;

	LOG_ALL(neuronsstackviewlog) << "got a key down event" << std::endl;

	if (signal.key == gui::keys::A) {

		_section = std::max((int)_neurons->getFirstSection(), _section - 1);

		LOG_ALL(neuronsstackviewlog) << "setting current section to " << _section << std::endl;

		_painter->setCurrentSection(_section);

		setDirty(_painter);
	}

	if (signal.key == gui::keys::D) {

		_section = std::min((int)_neurons->getLastSection(), _section + 1);

		LOG_ALL(neuronsstackviewlog) << "setting current section to " << _section << std::endl;

		_painter->setCurrentSection(_section);

		setDirty(_painter);
	}

	if (signal.key == gui::keys::E) {

		_painter->showEnds(true);
		_painter->showContinuations(false);
		_painter->showBranches(false);

		setDirty(_painter);
	}

	if (signal.key == gui::keys::C) {

		_painter->showEnds(false);
		_painter->showContinuations(true);
		_painter->showBranches(false);

		setDirty(_painter);
	}

	if (signal.key == gui::keys::B) {

		_painter->showEnds(false);
		_painter->showContinuations(false);
		_painter->showBranches(true);

		setDirty(_painter);
	}

	if (signal.key == gui::keys::S) {

		_painter->showEnds(true);
		_painter->showContinuations(true);
		_painter->showBranches(true);

		setDirty(_painter);
	}

	if (signal.key == gui::keys::O) {

		_painter->showAllNeurons();

		setDirty(_painter);
	}

	if (signal.key == gui::keys::N) {

		_painter->showSliceIds(signal.modifiers & gui::keys::ShiftDown);
	}

	if (signal.key == gui::keys::Q) {

		_painter->showCompleteNeurons(signal.modifiers & gui::keys::ShiftDown);

		setDirty(_painter);
	}

	if (signal.key == gui::keys::Tab) {

		_painter->setAlpha(_alpha);
		if (_alpha == 1)
			_alpha = 0;
		else if (_alpha == 0)
			_alpha = 0.8;
		else
			_alpha = 1;

		setDirty(_painter);
	}
}

void
NeuronsStackView::onMouseDown(gui::MouseDown& signal) {

	if (!_neurons.isSet())
		return;

	if (!_painter->getSize().contains(signal.position))
		return;

	if (signal.button == gui::buttons::Left) {

		// find the neuron that was clicked at

		unsigned int interSectionInterval = _section;
		util::point<double> center = signal.position;

		boost::shared_ptr<SegmentTree> closestNeuron;
		double minDistance = std::numeric_limits<double>::max();

		foreach (boost::shared_ptr<SegmentTree> neuron, *_neurons) {

			// find the closest segments
			std::vector<std::pair<boost::shared_ptr<EndSegment>, double> > closestEnds =
					neuron->findEnds(center, interSectionInterval, 1e6);
			std::vector<std::pair<boost::shared_ptr<ContinuationSegment>, double> > closestContinuations =
					neuron->findContinuations(center, interSectionInterval, 1e6);
			std::vector<std::pair<boost::shared_ptr<BranchSegment>, double> > closestBranches =
					neuron->findBranches(center, interSectionInterval, 1e6);

			// get the distance of the current neuron to the click position
			double distance =
					std::min(
						(closestEnds.size() > 0 ? closestEnds[0].second : std::numeric_limits<double>::max()),
						std::min(
							(closestContinuations.size() > 0 ? closestContinuations[0].second : std::numeric_limits<double>::max()),
							(closestBranches.size() > 0 ? closestBranches[0].second : std::numeric_limits<double>::max())
						)
					);

			if (distance < minDistance) {

				minDistance = distance;
				closestNeuron = neuron;
			}
		}

		if (closestNeuron) {

			if (_selectedNeurons->contains(closestNeuron))
				_selectedNeurons->remove(closestNeuron);
			else
				_selectedNeurons->add(closestNeuron);
		}

		setDirty(_selection);
		setDirty(_painter);
	}

	if (signal.button == gui::buttons::Right) {

		_selectedNeurons->clear();

		setDirty(_selection);
		setDirty(_painter);
	}
}
