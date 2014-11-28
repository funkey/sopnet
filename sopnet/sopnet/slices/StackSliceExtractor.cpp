#include <imageprocessing/ComponentTreeExtractor.h>
#include <sopnet/features/Overlap.h>
#include <util/ProgramOptions.h>
#include "ComponentTreeConverter.h"
#include "StackSliceExtractor.h"

static logger::LogChannel stacksliceextractorlog("stacksliceextractorlog", "[StackSliceExtractor] ");

util::ProgramOption optionSimilarityThreshold(
		util::_module           = "sopnet.slices",
		util::_long_name        = "similarityThreshold",
		util::_description_text = "The minimum normalized overlap [#overlap/(#size1 + #size2 - #overlap)] for which two slices are considered the same.",
		util::_default_value    = 0.75);

util::ProgramOption optionSetDifferenceThreshold(
		util::_module           = "sopnet.slices",
		util::_long_name        = "setDifferenceThreshold",
		util::_description_text = "The maximal set difference for which two slices are considered the same.",
		util::_default_value    = 200);

StackSliceExtractor::StackSliceExtractor(unsigned int section, float resX, float resY, float resZ) :
	_section(section),
	_resX(resX),
	_resY(resY),
	_resZ(resZ),
	_sliceImageExtractor(boost::make_shared<ImageExtractor>()),
	_cteParameters(boost::make_shared<ComponentTreeExtractorParameters>()),
	_sliceCollector(boost::make_shared<SliceCollector>()) {

	registerInput(_sliceImageStack, "slices");
	registerInput(_forceExplanation, "force explanation");
	registerOutput(_sliceCollector->getOutput("slices"), "slices");
	registerOutput(_sliceCollector->getOutput("conflict sets"), "conflict sets");

	_sliceImageStack.registerCallback(&StackSliceExtractor::onInputSet, this);

	// set default cte parameters from program options
	_cteParameters->darkToBright = false;
	_cteParameters->minSize      = 0;
	_cteParameters->maxSize      = 100000000;
}

void
StackSliceExtractor::onInputSet(const pipeline::InputSet<ImageStack>&) {

	LOG_DEBUG(stacksliceextractorlog) << "image stack set" << std::endl;

	// connect input image stack to slice image extractor
	_sliceImageExtractor->setInput(_sliceImageStack.getAssignedOutput());

	// clear slice collector content
	_sliceCollector->clearInputs(0);

	// for each image in the stack, set up the pipeline
	for (unsigned int i = 0; i < _sliceImageStack->size(); i++) {

		boost::shared_ptr<ComponentTreeExtractor<unsigned char> > cte = boost::make_shared<ComponentTreeExtractor<unsigned char> >();

		cte->setInput("image", _sliceImageExtractor->getOutput(i));
		cte->setInput("parameters", _cteParameters);

		boost::shared_ptr<ComponentTreeConverter> converter = boost::make_shared<ComponentTreeConverter>(_section, _resX, _resY, _resZ);

		converter->setInput(cte->getOutput());

		_sliceCollector->addInput(converter->getOutput());
	}

	LOG_DEBUG(stacksliceextractorlog) << "internal pipeline set up" << std::endl;
}

StackSliceExtractor::SliceCollector::SliceCollector() :
	_allSlices(new Slices()),
	_conflictSets(new ConflictSets()) {

	registerInputs(_slices, "slices");
	registerOutput(_allSlices, "slices");
	registerOutput(_conflictSets, "conflict sets");
}

void
StackSliceExtractor::SliceCollector::updateOutputs() {

	/*
	 * initialise
	 */

	_allSlices->clear();
	_conflictSets->clear();

	// create a copy of the input slice collections
	std::vector<Slices> inputSlices;
	foreach (boost::shared_ptr<Slices> slices, _slices)
		inputSlices.push_back(*slices);

	// remove all duplicates from the slice collections
	inputSlices = removeDuplicates(inputSlices);

	// create outputs
	extractSlices(inputSlices);
	extractConstraints(inputSlices);

	LOG_DEBUG(stacksliceextractorlog) << _allSlices->size() << " slices found" << std::endl;
	LOG_DEBUG(stacksliceextractorlog) << _conflictSets->size() << " conflict sets found" << std::endl;
}

unsigned int
StackSliceExtractor::SliceCollector::countSlices(const std::vector<Slices>& slices) {

	unsigned int numSlices = 0;

	for (unsigned int level = 0; level < slices.size(); level++)
		numSlices += slices[level].size();

	return numSlices;
}

std::vector<Slices>
StackSliceExtractor::SliceCollector::removeDuplicates(const std::vector<Slices>& slices) {

	LOG_DEBUG(stacksliceextractorlog) << "removing duplicates from " << countSlices(slices) << " slices" << std::endl;

	unsigned int oldSize = 0;
	unsigned int newSize = countSlices(slices);

	std::vector<Slices> withoutDuplicates = slices;

	while (oldSize != newSize) {

		LOG_DEBUG(stacksliceextractorlog) << "current size is " << countSlices(withoutDuplicates) << std::endl;
	
		withoutDuplicates = removeDuplicatesPass(withoutDuplicates);

		LOG_DEBUG(stacksliceextractorlog) << "new size is " << countSlices(withoutDuplicates) << std::endl;

		oldSize = newSize;
		newSize = countSlices(withoutDuplicates);
	}

	LOG_DEBUG(stacksliceextractorlog) << "removed " << (countSlices(slices) - countSlices(withoutDuplicates)) << " slices" << std::endl;

	return withoutDuplicates;
}

std::vector<Slices>
StackSliceExtractor::SliceCollector::removeDuplicatesPass(const std::vector<Slices>& allSlices) {

	std::vector<Slices> slices = allSlices;

	Overlap normalizedOverlap(true /* normalize */, false /* don't align */);

	double overlapThreshold             = optionSimilarityThreshold;
	unsigned int setDifferenceThreshold = optionSetDifferenceThreshold;

	// for all levels
	for (unsigned int level = 0; level < slices.size(); level++) { 

		// for each slice
		foreach (boost::shared_ptr<Slice> slice, slices[level]) {

			std::vector<boost::shared_ptr<Slice> > duplicates;

			// for all sub-levels
			for (unsigned int subLevel = level + 1; subLevel < slices.size(); subLevel++) {

				std::vector<boost::shared_ptr<Slice> > toBeRemoved;

				// for each slice
				foreach (boost::shared_ptr<Slice> subSlice, slices[subLevel]) {

					// if the overlap exceeds the threshold...
					if (normalizedOverlap.exceeds(*slice, *subSlice, overlapThreshold)) {

						// get the set difference
						Overlap nonNormalizedOverlap(false, false);
						int overlap = nonNormalizedOverlap(*slice, *subSlice);
						int sliceSize = slice->getComponent()->getSize();
						int subSliceSize = subSlice->getComponent()->getSize();

						// ...and the set difference is small enough, store 
						// subSlice as aduplicate of slice
						unsigned int setDifference = (sliceSize - overlap) + (subSliceSize - overlap);

						if (setDifference < setDifferenceThreshold) {

							duplicates.push_back(subSlice);
							toBeRemoved.push_back(subSlice);
						}
					}
				}

				// remove duplicates from this level
				foreach (boost::shared_ptr<Slice> subSlice, toBeRemoved)
					slices[subLevel].remove(subSlice);
			}

			// replace slice and duplicates by their intersection
			foreach (boost::shared_ptr<Slice> duplicate, duplicates) {

				LOG_ALL(stacksliceextractorlog)
						<< "intersecting " << slice->getId()
						<< " and " << duplicate->getId()
						<< std::endl;

				LOG_ALL(stacksliceextractorlog)
						<< "previous size was " << slice->getComponent()->getSize()
						<< std::endl;

				slice->intersect(*duplicate);

				LOG_ALL(stacksliceextractorlog)
						<< "new size is " << slice->getComponent()->getSize()
						<< std::endl;
			}
		}
	}

	return slices;
}

void
StackSliceExtractor::SliceCollector::extractSlices(const std::vector<Slices>& slices) {

	// for all levels
	for (unsigned int level = 0; level < slices.size(); level++)
		_allSlices->addAll(slices[level]);
}

void
StackSliceExtractor::SliceCollector::extractConstraints(const std::vector<Slices>& slices) {

	Overlap overlap(false /* don't normlize */, false /* don't align */);

	std::vector<unsigned int> conflictIds(2);

	// for all levels
	for (unsigned int level = 0; level < slices.size(); level++) { 

		// for each slice
		foreach (boost::shared_ptr<Slice> slice, slices[level]) {

			unsigned int numOverlaps = 0;

			// for all sub-levels
			for (unsigned int subLevel = level + 1; subLevel < slices.size(); subLevel++) {

				// for each slice
				foreach (boost::shared_ptr<Slice> subSlice, slices[subLevel]) {

					// if there is overlap, add a consistency constraint
					if (overlap.exceeds(*slice, *subSlice, 0)) {

						conflictIds[0] = slice->getId();
						conflictIds[1] = subSlice->getId();

						_allSlices->addConflicts(conflictIds);

						ConflictSet conflictSet;
						conflictSet.addSlice(slice->getId());
						conflictSet.addSlice(subSlice->getId());

						_conflictSets->add(conflictSet);
					}
				}
			}

			// if there is no overlap with other slices, make sure that this
			// slice will be picked at most once
			if (numOverlaps == 0) {

				ConflictSet conflictSet;
				conflictSet.addSlice(slice->getId());

				_conflictSets->add(conflictSet);
			}
		}
	}
}
