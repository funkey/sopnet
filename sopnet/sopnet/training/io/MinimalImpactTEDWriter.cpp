#include "MinimalImpactTEDWriter.h" 
#include <iostream>
#include <fstream>
#include <algorithm>

MinimalImpactTEDWriter::MinimalImpactTEDWriter() :
	_rNeuronExtractor(boost::make_shared<NeuronExtractor>()) {
	
	registerInput(_goldStandard, "gold standard");
	registerInput(_linearConstraints, "linear constraints");
	registerInput(_segments, "segments");

}

void
MinimalImpactTEDWriter::write(std::string filename) {

	// Create the internal pipeline
	createPipeline();

	updateInputs();

	// Loop through segments
		// Introduce constraints that flip them compared to the goldstandard
		// Write the resulting TED to file (check and maybe change file format)

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
	// IdMapCreator [reconstruction] ----> TED
	_teDistance->setInput("reconstruction", _rimCreator->getOutput("id map"));
	// Reconstructor ----> NeuronExtractor [reconstruction]
	_rNeuronExtractor->setInput("segments", _rReconstructor->getOutput("reconstruction"));
	// NeuronExtractor [reconstruction] ----> IdMapCreator [reconstruction]
	_rimCreator->setInput("neurons", _rNeuronExtractor->getOutput("neurons"));
	// _rimCreator might also need a reference, something like below.
	//resultIdMapCreator->setInput("reference", rawSectionsReader->getOutput());
	// Linear Solver ----> Reconstructor
	_rReconstructor->setInput("solution", _linearSolver->getOutput("solution"));
	// -- Segments --> Reconstructor
	_rReconstructor->setInput("segments",_segments);
	// -- Linear Constraints --> Linear Solver
	_linearSolver->setInput("linear constraints",_linearConstraints);

}
