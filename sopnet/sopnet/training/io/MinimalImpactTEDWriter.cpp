#include <iostream>
#include <fstream>
#include <algorithm>
#include <stdio.h>

#include <boost/timer/timer.hpp>

#include <imageprocessing/SubStackSelector.h>
#include <util/ProgramOptions.h>
#include "MinimalImpactTEDWriter.h" 

util::ProgramOption optionNumAdjacentSections(
		util::_module           = "sopnet.training",
		util::_long_name        = "numAdjacentSections",
		util::_default_value    = 0,
		util::_description_text = "The number of adjacent sections to consider for the computation of the minimal impact TED coefficients. If set to 0 (default), all sections will be considered.");

util::ProgramOption optionLimitToISI(
		util::_module           = "sopnet.training",
		util::_long_name        = "limitToISI",
		util::_description_text = "If set, limits the computation of the TED coefficients to segments in the given inter-section interval.");

util::ProgramOption optionWriteTedConditions(
		util::_module           = "sopnet.training",
		util::_long_name        = "writeTedConditions",
		util::_description_text = "Instead of computing the TED coefficients, write TED conditions to file minimalImapctTEDconditions.txt: "
		                          "These are the individual solutions that are used to compute the TED for a pinned segment variable.");

util::ProgramOption optionUseDirectGroundTruth(
		util::_module           = "sopnet.training",
		util::_long_name        = "useDirectGroundTruth",
		util::_description_text = "For the computation of the TED coefficients, use the ground-truth directly instead of the gold-standard.");

static logger::LogChannel minimalImpactTEDlog("minimalImpactTEDlog", "[minimalImapctTED] ");

MinimalImpactTEDWriter::MinimalImpactTEDWriter() :
	_teDistance(boost::make_shared<TolerantEditDistance>()),
	_randIndex(boost::make_shared<RandIndex>()),
	_voi(boost::make_shared<VariationOfInformation>()),
	_gsimCreator(boost::make_shared<IdMapCreator>()),
	_rimCreator(boost::make_shared<IdMapCreator>()),
	_rNeuronExtractor(boost::make_shared<NeuronExtractor>()),
	_gsNeuronExtractor(boost::make_shared<NeuronExtractor>()),
	_rReconstructor(boost::make_shared<Reconstructor>()),
	_linearSolver(boost::make_shared<LinearSolver>()),
	_objectiveGenerator(boost::make_shared<ObjectiveGenerator>()),
	_hammingCostFunction(boost::make_shared<HammingCostFunction>())
	{
	
	registerInput(_goldStandard, "gold standard");
	registerInput(_groundTruth, "ground truth");
	registerInput(_linearConstraints, "linear constraints");
	registerInput(_segments, "segments");
	registerInput(_reference, "reference");
	registerInput(_problemConfiguration, "problem configuration");

}

void
MinimalImpactTEDWriter::write(std::string filename, std::string measure) {

	updateInputs();

	initPipeline();

	int limitToISI = -1;
	if (optionLimitToISI)
		limitToISI = optionLimitToISI.as<int>() - SliceHashConfiguration::sectionOffset;

	if (optionWriteTedConditions)
		filename = "minimalImpactTEDconditions.txt";

	// append ISI number to output filename
	if (limitToISI >= 0)
		filename = filename + "_" + optionLimitToISI.as<std::string>();

	if (optionWriteTedConditions) {
		LOG_USER(minimalImpactTEDlog) << "writing ted conditions to " << filename << std::endl;
	} else {
		LOG_USER(minimalImpactTEDlog) << "writing ted coefficients to " << filename << std::endl;
	}

	// Remove the old file
	if( remove( filename.c_str() ) != 0 ) {
		LOG_DEBUG(minimalImpactTEDlog) << "Old file to output minimal impact TED approximation sucessfully deleted." << std::endl;
	} 
	else {
		LOG_DEBUG(minimalImpactTEDlog) << "Could not delete old file to output minimal impact TED approximation." << std::endl;
	}

	// Open file for writing
	std::ofstream outfile;

	outfile.open(filename.c_str());
	
	 // Get a vector with all gold standard segments.
	const std::vector<boost::shared_ptr<Segment> > goldStandard = _goldStandard->getSegments();

	int constant = 0;

	// Loop through variables
	std::set<unsigned int> variables = _problemConfiguration->getVariables();

	LOG_USER(minimalImpactTEDlog) << "computing ted coefficients for " << variables.size() << " variables" << std::endl;

	if (optionWriteTedConditions) {

		outfile << "# hash: [hashes of flipped segments]" << std::endl;

	} else {

		outfile << "numVar " << variables.size() << std::endl;
		outfile << "# var_num costs # hash value_in_gs fs fm fp fn ( <- when flipped )" << std::endl;
	}

	std::set<unsigned int> goldStandardIds;
	foreach (boost::shared_ptr<Segment> s, goldStandard)
		goldStandardIds.insert(s->getId());

	// create a map from segment ids to segment pointers
	std::map<unsigned int, boost::shared_ptr<Segment> > idToSegment;
	foreach (boost::shared_ptr<Segment> segment, _segments->getSegments())
		idToSegment[segment->getId()] = segment;

	for ( unsigned int varNum = 0 ; varNum < variables.size() ; varNum++ ) {

		unsigned int segmentId = _problemConfiguration->getSegmentId(varNum);

		int interSectionInterval = _problemConfiguration->getInterSectionInterval(varNum);

		if (limitToISI >= 0)
			if (interSectionInterval != limitToISI)
				continue;

		std::string timerMessage = "\n\nMinimalImpactTEDWriter: variable " + boost::lexical_cast<std::string>(varNum) + ", %ws\n\n";
		boost::timer::auto_cpu_timer timer(timerMessage);

		// re-create the pipeline for the current segment and its inter-section 
		// interval
		if (!optionWriteTedConditions)
			updatePipeline(interSectionInterval, optionNumAdjacentSections.as<int>());
	
		// Is the segment that corresponds to the variable part of the gold standard?
		bool isContained = (goldStandardIds.count(segmentId) > 0);

		// get the hash value of this segment
		SegmentHash segmentHash = idToSegment[segmentId]->hashValue();

		// pin the value of the current variable to its opposite
		_linearSolver->pinVariable(varNum, (isContained ? 0 : 1));

		if (!optionWriteTedConditions) {

			if (measure == "ted") {

				pipeline::Value<TolerantEditDistanceErrors> errors = _teDistance->getOutput("errors");
				int sumErrors = errors->getNumSplits() + errors->getNumMerges() + errors->getNumFalsePositives() + errors->getNumFalseNegatives();

				outfile << "c" << varNum << " ";
				outfile << (isContained ? -sumErrors : sumErrors) << " ";
				outfile << "# ";
				outfile << segmentHash << " ";
				outfile << (isContained ? 1 : 0) << " ";
				outfile << errors->getNumSplits() << " ";
				outfile << errors->getNumMerges() << " ";
				outfile << errors->getNumFalsePositives() << " ";
				outfile << errors->getNumFalseNegatives() << std::endl;

				if (isContained) {

					// Forced segment to not be part of the reconstruction.
					// This resulted in a number of errors that are going to be stored in the constant.
					// To make net 0 errors when the variable is on, minus the number of errors will be written to the file.

					constant += sumErrors;
				}

			} else if (measure == "voi") {

				pipeline::Value<VariationOfInformationErrors> errors = _voi->getOutput("errors");
				double error = errors->getSplitEntropy() + errors->getMergeEntropy();

				outfile << "c" << varNum << " ";
				outfile << (isContained ? -error: error) << " ";
				outfile << "# ";
				outfile << segmentHash << " ";
				outfile << (isContained ? 1 : 0) << std::endl;

				if (isContained)
					constant += error;

			} else if (measure == "rand") {

				pipeline::Value<RandIndexErrors> errors = _randIndex->getOutput("errors");
				double error = 1.0 - errors->getRandIndex();

				outfile << "c" << varNum << " ";
				outfile << (isContained ? -error: error) << " ";
				outfile << "# ";
				outfile << segmentHash << " ";
				outfile << (isContained ? 1 : 0) << std::endl;

				if (isContained)
					constant += error;
			}

		} else {

			pipeline::Value<Solution> solution = _linearSolver->getOutput("solution");

			outfile << segmentHash << ":";
			for (unsigned int i = 0; i < variables.size(); i++) {

				unsigned int id = _problemConfiguration->getSegmentId(i);

				// was flipped?
				if ((*solution)[i] != goldStandardIds.count(id))
					outfile << " " << idToSegment[id]->hashValue();
			}
			outfile << std::endl;
		}

		// Remove constraint
		_linearSolver->unpinVariable(varNum);
	}

	if (!optionWriteTedConditions) {

		// Write constant to file
		outfile << "constant " << constant << std::endl;
	}

	outfile.close();
}

void
MinimalImpactTEDWriter::initPipeline() {

	// Here we assemble the static part of the pipeline, i.e., the parts that 
	// don't change between iterations. Currently, these are all parts below the 
	// linear solver that creates new reconstructions based on pinned variables; 
	// and everything below the gold standard id map creator.

	_hammingCostFunction = boost::make_shared<HammingCostFunction>();
	_objectiveGenerator = boost::make_shared<ObjectiveGenerator>();
	_linearSolver = boost::make_shared<LinearSolver>();

	_gsNeuronExtractor = boost::make_shared<NeuronExtractor>();

	// -- Gold Standard --> Hamming Cost Function
	_hammingCostFunction->setInput("gold standard", _goldStandard);

	// -- Segments --> Objective Generator
	_objectiveGenerator->setInput("segments", _segments);
	// Hamming Cost Function ----> Objective Generator
	_objectiveGenerator->addInput("cost functions", _hammingCostFunction->getOutput());

	// -- Linear Constraints --> Linear Solver
	_linearSolver->setInput("linear constraints",_linearConstraints);
	// -- Parameters --> Linear Solver
	_linearSolver->setInput("parameters", boost::make_shared<LinearSolverParameters>(Binary));
	// Objective Generator ----> Linear Solver
	_linearSolver->setInput("objective", _objectiveGenerator->getOutput());

	// -- gold standard --> NeuronExtractor [gold standard]
	_gsNeuronExtractor->setInput("segments", _goldStandard);

	if (!optionUseDirectGroundTruth) {

		_gsimCreator = boost::make_shared<IdMapCreator>();
		// NeuronExtractor [gold standard] ----> IdMapCreator [gold standard]
		_gsimCreator->setInput("neurons", _gsNeuronExtractor->getOutput("neurons"));
		// reference image stack for width height and size of output image stacks
		// -- reference --> IdMapCreator
		_gsimCreator->setInput("reference", _reference);
	}
}

void
MinimalImpactTEDWriter::updatePipeline(int interSectionInterval, int numAdjacentSections) {

	boost::timer::auto_cpu_timer timer("\tupdatePipeline()\t\t\t%ws\n");

	// create new pipeline components
	_teDistance = boost::make_shared<TolerantEditDistance>();
	_randIndex = boost::make_shared<RandIndex>();
	_voi = boost::make_shared<VariationOfInformation>();
	_rimCreator = boost::make_shared<IdMapCreator>();
	_rNeuronExtractor = boost::make_shared<NeuronExtractor>();
	_rReconstructor = boost::make_shared<Reconstructor>();

	// find the range of sections to consider for the computation of the TED
	int minSection = interSectionInterval - numAdjacentSections;
	int maxSection = interSectionInterval + numAdjacentSections - 1;

	// pipeline values for the image stacks used to compute the TED
	pipeline::Value<ImageStack> goldStandard;
	pipeline::Value<ImageStack> reconstruction;

	// get image stacks for TED either as subsets (if numAdjacentSections is 
	// set) or as the original stacks
	if (numAdjacentSections > 0) {

		pipeline::Process<SubStackSelector> goldStandardSelector(minSection, maxSection);
		pipeline::Process<SubStackSelector> reconstructionSelector(minSection, maxSection);

		if (optionUseDirectGroundTruth)
			goldStandardSelector->setInput(_groundTruth);
		else
			goldStandardSelector->setInput(_gsimCreator->getOutput("id map"));
		reconstructionSelector->setInput(_rimCreator->getOutput("id map"));

		goldStandard   = goldStandardSelector->getOutput();
		reconstruction = reconstructionSelector->getOutput();

	} else {

		if (optionUseDirectGroundTruth)
			goldStandard = _groundTruth;
		else
			goldStandard = _gsimCreator->getOutput("id map");
		reconstruction = _rimCreator->getOutput("id map");
	}

	// Set inputs

	// The name ground truth is slightly misleading here because really we are setting the gold standard as input,
	// but that is what the correct input in the TED is called.
	// IdMapCreator [gold standard] ----> TED
	_teDistance->setInput("ground truth", goldStandard);
	// IdMapCreator [reconstruction] ----> TED
	_teDistance->setInput("reconstruction", reconstruction);
	_randIndex->setInput("stack 1", goldStandard);
	_randIndex->setInput("stack 2", reconstruction);
	_voi->setInput("stack 1", goldStandard);
	_voi->setInput("stack 2", reconstruction);
	// Reconstructor ----> NeuronExtractor [reconstruction]
	_rNeuronExtractor->setInput("segments", _rReconstructor->getOutput("reconstruction"));
	// NeuronExtractor [reconstruction] ----> IdMapCreator [reconstruction]
	_rimCreator->setInput("neurons", _rNeuronExtractor->getOutput("neurons"));
	// reference image stack for width height and size of output image stack
	// -- reference --> IdMapCreator
	_rimCreator->setInput("reference",_reference);
	// Linear Solver ----> Reconstructor
	_rReconstructor->setInput("solution", _linearSolver->getOutput("solution"));
	// -- Segments --> Reconstructor
	_rReconstructor->setInput("segments",_segments);
}
