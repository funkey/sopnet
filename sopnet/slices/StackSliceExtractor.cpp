#include <external/kdtree++/kdtree.hpp>

#include <imageprocessing/ComponentTree.h>
#include <imageprocessing/Mser.h>
#include <sopnet/features/Overlap.h>
#include <util/ProgramOptions.h>
#include "ComponentTreeConverter.h"
#include "StackSliceExtractor.h"

static logger::LogChannel stacksliceextractorlog("stacksliceextractorlog", "[StackSliceExtractor] ");

util::ProgramOption optionMaxSliceComparisonDistance(
		util::_module           = "sopnet",
		util::_long_name        = "maxSliceComparisonDistance",
		util::_description_text = "The maximal center distance between two slices to check them for consistency.",
		util::_default_value    = 1000);

StackSliceExtractor::StackSliceExtractor(unsigned int section) :
	_section(section),
	_sliceImageExtractor(boost::make_shared<ImageExtractor>()),
	_mserParameters(boost::make_shared<MserParameters>()),
	_sliceCollector(boost::make_shared<SliceCollector>()) {

	registerInput(_sliceImageStack, "slices");
	registerInput(_forceExplanation, "force explanation");
	registerOutput(_sliceCollector->getOutput("slices"), "slices");
	registerOutput(_sliceCollector->getOutput("linear constraints"), "linear constraints");

	_sliceImageStack.registerBackwardCallback(&StackSliceExtractor::onInputSet, this);

	// set default mser parameters from program options
	_mserParameters->darkToBright = false;
	_mserParameters->brightToDark = true;
	_mserParameters->minArea      = 0;
	_mserParameters->maxArea      = 100000000;
}

void
StackSliceExtractor::onInputSet(const pipeline::InputSet<ImageStack>& signal) {

	LOG_DEBUG(stacksliceextractorlog) << "image stack set" << std::endl;

	// connect input image stack to slice image extractor
	_sliceImageExtractor->setInput(_sliceImageStack.getAssignedOutput());

	// clear slice collector content
	_sliceCollector->clearInputs(0);

	// for each image in the stack, set up the pipeline
	for (int i = 0; i < _sliceImageStack->size(); i++) {

		boost::shared_ptr<Mser> mser = boost::make_shared<Mser>();

		mser->setInput("image", _sliceImageExtractor->getOutput(i));
		mser->setInput("parameters", _mserParameters);

		boost::shared_ptr<ComponentTreeConverter> converter = boost::make_shared<ComponentTreeConverter>(_section);

		converter->setInput(mser->getOutput());

		_sliceCollector->addInput(converter->getOutput());
	}

	LOG_DEBUG(stacksliceextractorlog) << "internal pipeline set up" << std::endl;
}

StackSliceExtractor::SliceCollector::SliceCollector() {

	registerInputs(_slices, "slices");
	registerOutput(_allSlices, "slices");
	registerOutput(_linearConstraints, "linear constraints");
}

void
StackSliceExtractor::SliceCollector::updateOutputs() {

	/*
	 * initialise
	 */

	_allSlices->clear();
	_linearConstraints->clear();

	/*
	 * collect all slices
	 */

	LOG_DEBUG(stacksliceextractorlog) << "collecting all slices..." << std::endl;

	foreach (boost::shared_ptr<Slices> slices, _slices)
		_allSlices->addAll(slices);

	LOG_DEBUG(stacksliceextractorlog) << _allSlices->size() << " slices found" << std::endl;

	/*
	 * create kd-trees for faster slice look-up
	 */

	LOG_DEBUG(stacksliceextractorlog) << "creating kd-trees for faster slice lookup..." << std::endl;

	typedef KDTree::KDTree<
			2,
			boost::shared_ptr<Slice>,
			boost::function<double(boost::shared_ptr<Slice>, size_t)> >
		slice_tree_type;

	// a kd-tree for each set of slices
	std::vector<slice_tree_type*> sliceTrees;

	// a coordinate accessor function
	SliceCoordinates sliceCoordinates;

	// create and fill trees
	foreach (boost::shared_ptr<Slices> slices, _slices) {

		slice_tree_type* sliceTree = new slice_tree_type(sliceCoordinates);

		foreach (boost::shared_ptr<Slice> slice, *slices)
			sliceTree->insert(slice);

		sliceTrees.push_back(sliceTree);
	}

	LOG_DEBUG(stacksliceextractorlog) << sliceTrees.size() << " kd-trees created" << std::endl;

	/*
	 * extract linear consistency constraints
	 */

	LOG_DEBUG(stacksliceextractorlog) << "extracting consistency constraints..." << std::endl;

	// for every set of slices
	for (int i = 0; i < _slices.size(); i++) {

		// for every slice in this set
		foreach (boost::shared_ptr<Slice> slice, *_slices[i]) {

			unsigned int numConflicts = 0;

			// for every subsequent set of slices
			for (int j = i + 1; j < _slices.size(); j++) {

				unsigned int numCloseSlices = sliceTrees[j]->count_within_range(slice, optionMaxSliceComparisonDistance);

				std::vector<boost::shared_ptr<Slice> > closeSlices;
				closeSlices.reserve(numCloseSlices);
				sliceTrees[j]->find_within_range(slice, optionMaxSliceComparisonDistance, std::back_inserter(closeSlices));

				// for all close slices to the current slice
				foreach (boost::shared_ptr<Slice> closeSlice, closeSlices) {

					Overlap overlap;
					unsigned int conflict[2];

					// if they overlap
					if (overlap(*slice, *closeSlice, false, false) > 0) {

						LinearConstraint linearConstraint;

						// add constraint: slice + closeSlice ≤ 1
						linearConstraint.setCoefficient(slice->getId(), 1.0);
						linearConstraint.setCoefficient(closeSlice->getId(), 1.0);
						linearConstraint.setRelation(LessEqual);
						linearConstraint.setValue(1.0);

						_linearConstraints->add(linearConstraint);

						// remember the conflict in the set of all slices
						std::vector<unsigned int> conflict(2);
						conflict[0] = slice->getId();
						conflict[1] = closeSlice->getId();
						_allSlices->addConflicts(conflict);

						numConflicts++;
					}
				}
			}

			// if no conflict was found for a slice, add a linear constraint
			// manually to ensure it will be picked at most once:
			LinearConstraint linearConstraint;

			linearConstraint.setCoefficient(slice->getId(), 1.0);
			linearConstraint.setRelation(LessEqual);
			linearConstraint.setValue(1.0);

			_linearConstraints->add(linearConstraint);
		}
	}

	LOG_DEBUG(stacksliceextractorlog) << _linearConstraints->size() << " consistency constraints found" << std::endl;

	/*
	 * clean up
	 */

	foreach (slice_tree_type* sliceTree, sliceTrees)
		delete sliceTree;
}

