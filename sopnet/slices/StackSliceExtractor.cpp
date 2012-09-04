#include <imageprocessing/ComponentTree.h>
#include <imageprocessing/Mser.h>
#include <sopnet/features/Overlap.h>
#include <util/ProgramOptions.h>
#include "ComponentTreeConverter.h"
#include "StackSliceExtractor.h"

static logger::LogChannel stacksliceextractorlog("stacksliceextractorlog", "[StackSliceExtractor] ");

util::ProgramOption optionSimilarityThreshold(
		util::_module           = "sopnet.slices",
		util::_long_name        = "similarityThreshold",
		util::_description_text = "The minimum normalized overlap [#overlap/max(#size1, #size2)] for which two slices are considered the same.",
		util::_default_value    = 0.75);

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

	// slice id to size map
	std::map<unsigned int, unsigned int> sizes;

	// get the width and height of the area covered by the slices
	unsigned int width  = 0;
	unsigned int height = 0;
	foreach (boost::shared_ptr<Slices> slices, _slices) {

		foreach (boost::shared_ptr<Slice> slice, *slices) {

			width  = std::max(static_cast<unsigned int>(slice->getComponent()->getBoundingBox().maxX + 1), width);
			height = std::max(static_cast<unsigned int>(slice->getComponent()->getBoundingBox().maxY + 1), height);
		}
	}

	LOG_ALL(stacksliceextractorlog)
			<< "slices cover an area of at most (0, 0, "
			<< width << ", " << height << ")" << std::endl;

	LOG_DEBUG(stacksliceextractorlog) << "creating slice id maps..." << std::endl;

	// create a slice id image per slice-level
	typedef vigra::MultiArray<2, int> id_map;
	std::vector<id_map> sliceIds(_slices.size());

	// initialise images
	foreach (id_map& map, sliceIds)
		map.reshape(id_map::size_type(width, height), -1);

	LOG_ALL(stacksliceextractorlog) << "writing slice ids..." << std::endl;

	// store slice ids in these images

	// for all levels
	for (unsigned int level = 0; level < _slices.size(); level++) {

		LOG_ALL(stacksliceextractorlog) << "entering level " << level << std::endl;

		// for all slices in the current level
		foreach (boost::shared_ptr<Slice> slice, *_slices[level]) {

			LOG_ALL(stacksliceextractorlog) << "processing slice " << slice->getId() << std::endl;
			LOG_ALL(stacksliceextractorlog) << "slice is of size " << slice->getComponent()->getSize() << std::endl;

			// store slice size
			sizes[slice->getId()] = slice->getComponent()->getSize();

			// write id to id-map
			foreach (util::point<unsigned int> pixel, slice->getComponent()->getPixels()) {

				if (pixel.x >= width || pixel.y >= height) {

					LOG_ERROR(stacksliceextractorlog)
							<< "trying to write to invalid position " << pixel << std::endl;
					LOG_ERROR(stacksliceextractorlog)
							<< "only positions up to (" << width << ", " << height << ") are allowed" << std::endl;

				} else {

					sliceIds[level](pixel.x, pixel.y) = slice->getId();
				}
			}
		}
	}

	LOG_DEBUG(stacksliceextractorlog) << "checking for overlap..." << std::endl;

	// set of slices that are already well represented by others
	std::set<unsigned int> duplicates;

	// for all levels
	for (unsigned int level = 0; level < _slices.size(); level++) { 

		LOG_ALL(stacksliceextractorlog) << "entering level " << level << std::endl;

		// for all slices in the current level
		foreach (boost::shared_ptr<Slice> slice, *_slices[level]) {

			LOG_ALL(stacksliceextractorlog) << "processing slice " << slice->getId() << std::endl;

			if (duplicates.count(slice->getId())) {

				LOG_ALL(stacksliceextractorlog) << "this slice is a duplicate of another one -- skipping it" << std::endl;
				continue;

			} else {

				// add current slice to all slices
				_allSlices->add(slice);
			}

			LOG_ALL(stacksliceextractorlog) << "slice is of size " << slice->getComponent()->getSize() << std::endl;

			/*
			 * find overlapping slices
			 */

			// number of overlapping pixels to other slices
			std::map<unsigned int, unsigned int> overlap;

			// for every level below the current level
			for (unsigned int subLevel = level + 1; subLevel < _slices.size(); subLevel++) {

				LOG_ALL(stacksliceextractorlog) << "entering sub-level " << subLevel << std::endl;

				// for every pixel of the current slice
				foreach (util::point<unsigned int> pixel, slice->getComponent()->getPixels()) {

					unsigned int value = sliceIds[subLevel](pixel.x, pixel.y);

					if (value >= 0) {

						if (!overlap.count(value))
							overlap[value] = 1;
						else
							overlap[value]++;
					}
				}
			}

			/*
			 * filter out slices that are too similar
			 */

			unsigned int size = slice->getComponent()->getSize();
			unsigned int numConstraints = 0;

			unsigned int otherSlice, numOverlap;
			foreach (boost::tie(otherSlice, numOverlap), overlap) {

				unsigned int otherSize = sizes[otherSlice];

				// if the slices are too similar ignore them (now and in future
				// iterations)
				if (static_cast<double>(numOverlap)/std::max(size, otherSize) >= optionSimilarityThreshold.as<double>()) {

					duplicates.insert(otherSlice);

				// otherwise, add a linear consistency constraint
				} else {

					LinearConstraint linearConstraint;
					linearConstraint.setCoefficient(otherSlice, 1.0);
					linearConstraint.setCoefficient(slice->getId(), 1.0);
					linearConstraint.setRelation(LessEqual);
					linearConstraint.setValue(1.0);

					_linearConstraints->add(linearConstraint);

					std::vector<unsigned int> conflicts(2);
					conflicts[0] = otherSlice;
					conflicts[1] = slice->getId();
					_allSlices->addConflicts(conflicts);

					numConstraints++;
				}

				// if no conflict was found for a slice, add a linear constraint
				// manually to ensure it will be picked at most once:
				if (numConstraints == 0) {

					LOG_ALL(stacksliceextractorlog) << "manually adding 'at most one' constraint" << std::endl;

					LinearConstraint linearConstraint;
					linearConstraint.setCoefficient(slice->getId(), 1.0);
					linearConstraint.setRelation(LessEqual);
					linearConstraint.setValue(1.0);

					_linearConstraints->add(linearConstraint);
				}
			}
		}
	}

	LOG_DEBUG(stacksliceextractorlog) << _allSlices->size() << " slices found" << std::endl;
	LOG_DEBUG(stacksliceextractorlog) << _linearConstraints->size() << " consistency constraints found" << std::endl;
}

