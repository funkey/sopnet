#include <util/ProgramOptions.h>
#include <imageprocessing/ComponentTree.h>
#include <imageprocessing/ComponentTreeDownSampler.h>
#include <imageprocessing/Mser.h>
#include "SliceExtractor.h"

static logger::LogChannel sliceextractorlog("sliceextractorlog", "[SliceExtractor] ");

util::ProgramOption optionInvertMembraneMaps(
		util::_module = "sopnet",
		util::_long_name = "invertMembraneMaps",
		util::_description_text = "Invert the meaning of the membrane map. The default "
		                          "(not inverting) is: bright pixel = hight membrane probability.");

SliceExtractor::SliceExtractor(unsigned int section) :
	_mser(boost::make_shared<Mser>()),
	_mserParameters(boost::make_shared<MserParameters>()),
	_downSampler(boost::make_shared<ComponentTreeDownSampler>()),
	_converter(boost::make_shared<ComponentTreeConverter>(section)) {

	registerInput(_mser->getInput("image"), "membrane");
	registerOutput(_converter->getOutput("slices"), "slices");
	registerOutput(_converter->getOutput("linear constraints"), "linear constraints");

	// setup internal pipeline

	_mserParameters->darkToBright =  optionInvertMembraneMaps;
	_mserParameters->brightToDark = !optionInvertMembraneMaps;
	_mserParameters->maxArea      = 400000;

	_mser->setInput("parameters", _mserParameters);
	_downSampler->setInput(_mser->getOutput());
	_converter->setInput(_downSampler->getOutput());
}

SliceExtractor::ComponentTreeConverter::ComponentTreeConverter(unsigned int section) :
	_linearConstraints(boost::make_shared<LinearConstraints>()),
	_section(section) {

	registerInput(_componentTree, "component tree");
	registerOutput(_slices, "slices");
	registerOutput(_linearConstraints, "linear constraints");
}

unsigned int
SliceExtractor::ComponentTreeConverter::getNextSliceId() {

	unsigned int id;
	
	{
		boost::mutex::scoped_lock lock(SliceIdMutex);

		id = NextSliceId;

		NextSliceId++;
	}

	return id;
}

void
SliceExtractor::ComponentTreeConverter::updateOutputs() {

	convert();
}

void
SliceExtractor::ComponentTreeConverter::convert() {

	LOG_DEBUG(sliceextractorlog) << "converting component tree to slices..." << std::endl;

	_slices->clear();

	_linearConstraints->clear();

	// skip the fake root
	foreach (boost::shared_ptr<ComponentTree::Node> node, _componentTree->getRoot()->getChildren())
		_componentTree->visit(node, *this);

	LOG_DEBUG(sliceextractorlog) << "extracted " << _slices->size() << " slices" << std::endl;
}

void
SliceExtractor::ComponentTreeConverter::visitNode(boost::shared_ptr<ComponentTree::Node> node) {

	unsigned int sliceId = getNextSliceId();

	_path.push_back(sliceId);

	boost::shared_ptr<ConnectedComponent> component = node->getComponent();

	_slices->add(boost::make_shared<Slice>(sliceId, _section, component));

	LOG_ALL(sliceextractorlog) << "extracted a slice at " << component->getCenter() << std::endl;

	// for leafs
	if (node->getChildren().size() == 0)
		addLinearConstraint();
}

void
SliceExtractor::ComponentTreeConverter::leaveNode(boost::shared_ptr<ComponentTree::Node> node) {

	_path.pop_back();
}

void
SliceExtractor::ComponentTreeConverter::addLinearConstraint() {

	LOG_ALL(sliceextractorlog) << "found a leaf node, creating a linear constraint for it" << std::endl;

	LinearConstraint constraint;

	foreach (unsigned int sliceId, _path)
		constraint.setCoefficient(sliceId, 1);

	constraint.setRelation(Equal);

	constraint.setValue(1);

	LOG_ALL(sliceextractorlog) << "add constraint " << constraint << std::endl;

	_linearConstraints->add(constraint);

	_slices->addConflicts(_path);
}

unsigned int SliceExtractor::ComponentTreeConverter::NextSliceId = 0;
boost::mutex SliceExtractor::ComponentTreeConverter::SliceIdMutex;
