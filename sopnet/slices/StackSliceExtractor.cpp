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

	unsigned int width  = 0;
	unsigned int height = 0;

	foreach (boost::shared_ptr<Slices> slices, _slices) {

		_allSlices->addAll(slices);

		// get the width and height of the area covered by the slices
		foreach (boost::shared_ptr<Slice> slice, *slices) {

			width  = std::max(static_cast<unsigned int>(slice->getComponent()->getBoundingBox().maxX + 1), width);
			height = std::max(static_cast<unsigned int>(slice->getComponent()->getBoundingBox().maxY + 1), height);
		}
	}

	LOG_DEBUG(stacksliceextractorlog) << _allSlices->size() << " slices found" << std::endl;

	/*
	 * extract linear consistency constraints
	 */

	LOG_DEBUG(stacksliceextractorlog) << "extracting consistency constraints..." << std::endl;

	// create a slice id image per slice-level
	typedef vigra::MultiArray<2, double> id_map;
	std::vector<id_map> sliceIds(_slices.size());

	// initialise images
	foreach (id_map map, sliceIds)
		map.reshape(id_map::size_type(width, height), -1);

	LOG_ALL(stacksliceextractorlog) << "writing slice ids..." << std::endl;

	// store slice ids in these images
	for (unsigned int level = 0; level < _slices.size(); level++) {

		LOG_ALL(stacksliceextractorlog) << "entering level " << level << std::endl;

		foreach (boost::shared_ptr<Slice> slice, *_slices[level]) {

			LOG_ALL(stacksliceextractorlog) << "processing slice " << slice->getId() << std::endl;
			LOG_ALL(stacksliceextractorlog) << "slice is of size " << slice->getComponent()->getSize() << std::endl;

			foreach (util::point<unsigned int> pixel, slice->getComponent()->getPixels())
				sliceIds[level](pixel.x, pixel.y) = slice->getId();
		}
	}

	LOG_ALL(stacksliceextractorlog) << "checking for overlap..." << std::endl;

	// create consistency constraints
	for (unsigned int level = 0; level < _slices.size(); level++) {

		LOG_ALL(stacksliceextractorlog) << "entering level " << level << std::endl;

		foreach (boost::shared_ptr<Slice> slice, *_slices[level]) {

			LOG_ALL(stacksliceextractorlog) << "processing slice " << slice->getId() << std::endl;
			LOG_ALL(stacksliceextractorlog) << "slice is of size " << slice->getComponent()->getSize() << std::endl;

			unsigned int numConstraints = 0;

			// set of ids of already processed conflicting slices
			std::set<unsigned int> processed;

			for (unsigned int subLevel = level + 1; subLevel < _slices.size(); subLevel++) {

				LOG_ALL(stacksliceextractorlog) << "entering sub-level " << subLevel << std::endl;

				foreach (util::point<unsigned int> pixel, slice->getComponent()->getPixels()) {

					double value = sliceIds[subLevel](pixel.x, pixel.y);

					if (value >= 0 && !processed.count(static_cast<unsigned int>(value))) {

						LOG_ALL(stacksliceextractorlog) << "adding constraint for slice " << value << std::endl;

						LinearConstraint linearConstraint;
						linearConstraint.setCoefficient(static_cast<unsigned int>(value), 1.0);
						linearConstraint.setCoefficient(slice->getId(), 1.0);
						linearConstraint.setRelation(LessEqual);
						linearConstraint.setValue(1.0);

						_linearConstraints->add(linearConstraint);

						numConstraints++;

						processed.insert(static_cast<unsigned int>(value));
					}
				}
			}

			LOG_ALL(stacksliceextractorlog) << "found " << numConstraints << " constraints" << std::endl;

			if (numConstraints == 0) {

				LOG_ALL(stacksliceextractorlog) << "manually adding 'at most one' constraint" << std::endl;

				// if no conflict was found for a slice, add a linear constraint
				// manually to ensure it will be picked at most once:
				LinearConstraint linearConstraint;
				linearConstraint.setCoefficient(slice->getId(), 1.0);
				linearConstraint.setRelation(LessEqual);
				linearConstraint.setValue(1.0);

				_linearConstraints->add(linearConstraint);
			}
		}
	}

	LOG_DEBUG(stacksliceextractorlog) << _linearConstraints->size() << " consistency constraints found" << std::endl;
}

