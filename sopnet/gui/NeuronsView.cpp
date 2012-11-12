#include <gui/RotateView.h>
#include <sopnet/gui/SegmentsView.h>
#include <util/Logger.h>
#include "NeuronsView.h"

logger::LogChannel neuronsviewlog("neuronsviewlog", "[NeuronsView] ");

NeuronsView::NeuronsView() {

	registerInput(_neurons, "neurons");
	registerOutput(_painter, "painter");
}

void
NeuronsView::updateOutputs() {

	LOG_DEBUG(neuronsviewlog) << "update requested -- recreating internal pipeline!" << std::endl;

	boost::shared_ptr<gui::ContainerView<gui::HorizontalPlacing> > container = boost::make_shared<gui::ContainerView<gui::HorizontalPlacing> >();

	foreach (boost::shared_ptr<Neuron> neuron, *_neurons) {

		LOG_ALL(neuronsviewlog) << "adding a neuron" << std::endl;

		boost::shared_ptr<SegmentsView>    neuronView = boost::make_shared<SegmentsView>();
		boost::shared_ptr<gui::RotateView> rotateView = boost::make_shared<gui::RotateView>();

		neuronView->setInput("segments", neuron);
		rotateView->setInput(neuronView->getOutput());
		container->addInput(rotateView->getOutput());
	}

	boost::shared_ptr<PainterAssigner> painterAssigner = boost::make_shared<PainterAssigner>();

	painterAssigner->setInput(container->getOutput());

	painterAssigner->assignTo(*_painter);
}
