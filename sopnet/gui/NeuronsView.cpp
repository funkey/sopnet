#include <gui/Buttons.h>
#include <gui/RotateView.h>
#include <sopnet/gui/SegmentsView.h>
#include <util/Logger.h>
#include "NeuronsView.h"

logger::LogChannel neuronsviewlog("neuronsviewlog", "[NeuronsView] ");

NeuronsView::NeuronsView() :
		_container(boost::make_shared<gui::ContainerView<gui::HorizontalPlacing> > ("neurons")),
		_neuronsChanged(true) {

	registerInput(_neurons, "neurons");
	registerInput(_errors, "errors", pipeline::Optional);
	registerOutput(_container->getOutput(), "painter");
	registerOutput(_currentNeuron, "current neuron");

	_neurons.registerBackwardCallback(&NeuronsView::onNeuronsModified, this);
}

void
NeuronsView::updateOutputs() {

	LOG_DEBUG(neuronsviewlog) << "update requested" << std::endl;

	if (_neuronsChanged) {

		LOG_DEBUG(neuronsviewlog) << "clearing container" << std::endl;

		_container->clearInputs("painters");

		unsigned int neuronNum = 0;
		foreach (boost::shared_ptr<Neuron> neuron, *_neurons) {

			LOG_ALL(neuronsviewlog) << "adding a neuron" << std::endl;

			boost::shared_ptr<SegmentsView>    neuronView = boost::make_shared<SegmentsView>("single neuron");
			boost::shared_ptr<gui::RotateView> rotateView = boost::make_shared<gui::RotateView>();

			boost::function<void(gui::MouseDown&)> callback = boost::bind(&NeuronsView::onMouseDownOnNeuron, this, _1, neuronNum);
			rotateView->getOutput().registerForwardCallback(callback, this, signals::Transparent);

			neuronView->setInput("segments", neuron);
			if (_errors)
				neuronView->setInput("errors", _errors);
			rotateView->setInput(neuronView->getOutput());
			_container->addInput(rotateView->getOutput());

			neuronNum++;
		}

		LOG_DEBUG(neuronsviewlog) << "done updating internal pipeline" << std::endl;

		_neuronsChanged = false;
	}
}

void
NeuronsView::onNeuronsModified(const pipeline::Modified&) {

	_neuronsChanged = true;
}

void
NeuronsView::onMouseDownOnNeuron(const gui::MouseDown& signal, unsigned int neuron) {

	if (signal.button == gui::buttons::Left) {

		LOG_DEBUG(neuronsviewlog) << "you clicked on neuron " << neuron << std::endl;

		*_currentNeuron = neuron;

		setDirty(_currentNeuron);
	}
}
