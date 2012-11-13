#include <gui/RotateView.h>
#include <sopnet/gui/SegmentsView.h>
#include <util/Logger.h>
#include "NeuronsView.h"

logger::LogChannel neuronsviewlog("neuronsviewlog", "[NeuronsView] ");

NeuronsView::NeuronsView() :
		_container(boost::make_shared<gui::ContainerView<gui::HorizontalPlacing> > ("neurons")) {

	registerInput(_neurons, "neurons");
	registerOutput(_container->getOutput(), "painter");
}

void
NeuronsView::updateOutputs() {

	LOG_DEBUG(neuronsviewlog) << "update requested" << std::endl;

	LOG_DEBUG(neuronsviewlog) << "clearing container" << std::endl;

	_container->clearInputs("painters");

	foreach (boost::shared_ptr<Neuron> neuron, *_neurons) {

		LOG_ALL(neuronsviewlog) << "adding a neuron" << std::endl;

		boost::shared_ptr<SegmentsView>    neuronView = boost::make_shared<SegmentsView>("single neuron");
		boost::shared_ptr<gui::RotateView> rotateView = boost::make_shared<gui::RotateView>();

		neuronView->setInput("segments", neuron);
		rotateView->setInput(neuronView->getOutput());
		_container->addInput(rotateView->getOutput());
	}

	LOG_DEBUG(neuronsviewlog) << "done updating internal pipeline" << std::endl;
}
