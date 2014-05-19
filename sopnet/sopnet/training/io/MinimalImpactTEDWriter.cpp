#include "MinimalImpactTEDWriter.h" 
#include <iostream>
#include <fstream>
#include <algorithm>

static logger::LogChannel minimalImpactTEDlog("minimalImpactTEDlog", "[minimalImapctTED] ");

MinimalImpactTEDWriter::MinimalImpactTEDWriter() :
	_teDistance(boost::make_shared<TolerantEditDistance>()),
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
	registerInput(_linearConstraints, "linear constraints");
	registerInput(_segments, "segments");
	registerInput(_reference, "reference");
	registerInput(_problemConfiguration, "problem configuration");

}

void
MinimalImpactTEDWriter::write(std::string filename) {

	// Create the internal pipeline
	createPipeline();

	updateInputs();

	 // Get a vector with all gold standard segments.
        const std::vector<boost::shared_ptr<Segment> > goldStandard = _goldStandard->getSegments();

	int constant = 0;

	LOG_DEBUG(minimalImpactTEDlog) << "in write function." << std::endl;

	// Loop through variables
	std::set<unsigned int> variables = _problemConfiguration->getVariables();
	for ( unsigned int varNum = 0 ; varNum < variables.size() ; varNum++ ) {
		
		// Introduce constraints that flip the segment corresponding to the ith variable
		// compared to the goldstandard.
	
		// Is the segment that corresponds to the variable part of the gold standard?
                unsigned int segmentId = _problemConfiguration->getSegmentId(varNum);

                bool isContained = false;
                foreach (boost::shared_ptr<Segment> s, goldStandard) {
                        if (s->getId() == segmentId) {
                                isContained = true;
                        }
                }
	
		LOG_DEBUG(minimalImpactTEDlog) << "is contained: " << isContained << std::endl;

		LinearConstraint constraint;
		constraint.setRelation(Equal);

                if (isContained == true) {
			// Force segment to not be used in reconstruction
			constraint.setCoefficient(varNum,1.0);
			constraint.setValue(0);
                }
                else {
			// Force segment to be used in reconstruction
			constraint.setCoefficient(varNum,1.0);
			constraint.setValue(1);
                }

		_linearConstraints->add(constraint);
		
		pipeline::Value<Errors> errors = _teDistance->getOutput("errors");
		unsigned int splitsAndMerges = errors->getNumSplits() + errors->getNumMerges();
		int splitsAndMergesInt = (int) splitsAndMerges;

		if (isContained == true) {
			// Forced segment to not be part of the reconstruction.
			// This resulted in a number of errors that are going to be stored in the constant.
			// To make net 0 errors when the variable is on, minus the number of errors will be written to the file.

			writeToFile(filename, -splitsAndMergesInt);

			constant += splitsAndMergesInt;
		}
		else {
			// Forced segment to be part of the reconstruction.
			// This resulted in a number of errors that are going to be written to the file.

			writeToFile(filename, splitsAndMergesInt);
		}

		// Remove constraint
		_linearConstraints->removeLastConstraint();
	}

	// Write constant to file
	writeToFile(filename, constant);
	
	LOG_DEBUG(minimalImpactTEDlog) << "Constant is: " << constant << std::endl;
}

void
MinimalImpactTEDWriter::writeToFile(std::string filename, int value) {

	std::ofstream outfile;
	outfile.open(filename.c_str(), std::ios_base::app);
	outfile << value << std::endl;
	outfile.close();

}

void
MinimalImpactTEDWriter::createPipeline() {

	// Set inputs

	// The name ground truth is slightly misleading here because really we are setting the gold standard as input,
	// but that is what the correct input in the TED is called.
	// IdMapCreator [gold standard] ----> TED
	_teDistance->setInput("ground truth", _gsimCreator->getOutput("id map"));
	// -- gold standard --> NeuronExtractor [gold standard]
	_gsNeuronExtractor->setInput("segments",_goldStandard);
	// NeuronExtractor [gold standard] ----> IdMapCreator [gold standard]
	_gsimCreator->setInput("neurons", _gsNeuronExtractor->getOutput("neurons"));
	// reference image stack for width height and size of output image stacks
	// -- reference --> IdMapCreator
	_gsimCreator->setInput("reference",_reference);
	// IdMapCreator [reconstruction] ----> TED
	_teDistance->setInput("reconstruction", _rimCreator->getOutput("id map"));
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
	// -- Linear Constraints --> Linear Solver
	_linearSolver->setInput("linear constraints",_linearConstraints);
	// -- Parameters --> Linear Solver
	_linearSolver->setInput("parameters", boost::make_shared<LinearSolverParameters>(Binary));
	// Objective Generator ----> Linear Solver
	_linearSolver->setInput("objective", _objectiveGenerator->getOutput());
	// -- Segments --> Objective Generator
        _objectiveGenerator->setInput("segments", _segments);
	// -- Gold Standard --> Hamming Cost Function
       	_hammingCostFunction->setInput("gold standard", _goldStandard);
	// Hamming Cost Function ----> Objective Generator
        _objectiveGenerator->addInput("cost functions", _hammingCostFunction->getOutput());

}
