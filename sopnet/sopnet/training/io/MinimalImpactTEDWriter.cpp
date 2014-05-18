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

}

void
MinimalImpactTEDWriter::write(std::string filename) {

	// Create the internal pipeline
	createPipeline();

	updateInputs();

	// Loop through segments
	for ( unsigned int i = 0; i < _segments->size(); i++ )
	{
		
		// Introduce constraints that flip the ith segment compared to the goldstandard
		// To do...

		// Write the resulting TED to file (check and maybe change file format)
		writeToFile(filename);
	}
	
}

void
MinimalImpactTEDWriter::writeToFile(std::string filename) {

	//_teDistance->getNumSplits()
	//_teDistance->getNumMerges()

	pipeline::Value<Errors> errors = _teDistance->getOutput("errors");

	std::ofstream outfile;
	outfile.open(filename.c_str(), std::ios_base::app);
	outfile << errors->getNumSplits()  << std::endl;
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
