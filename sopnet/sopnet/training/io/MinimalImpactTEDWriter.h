#ifndef SOPNET_MINIMAL_IMPACT_TED_WRITER_H__
#define SOPNET_MINIMAL_IMPACT_TED_WRITER_H__

#include <pipeline/all.h>
#include <sopnet/inference/LinearConstraints.h>
#include <sopnet/inference/ProblemConfiguration.h>
#include <sopnet/segments/Segments.h>
#include <sopnet/io/IdMapCreator.h>
#include <sopnet/evaluation/TolerantEditDistance.h>
#include <sopnet/neurons/NeuronExtractor.h>
#include <sopnet/inference/Reconstructor.h>
#include <sopnet/inference/LinearSolver.h>
#include <sopnet/inference/LinearConstraint.h>
#include <sopnet/inference/LinearConstraints.h>
#include <sopnet/inference/LinearSolverParameters.h>
#include <sopnet/inference/ObjectiveGenerator.h>
#include <sopnet/training/HammingCostFunction.h>

class MinimalImpactTEDWriter : public pipeline::SimpleProcessNode<> {

public:

	MinimalImpactTEDWriter();

	void write(std::string filename);

private:

	void updateOutputs() {}

	void clearPipeline();

	void createPipeline();

	/*********
	* Inputs *
	*********/

	// The gold standard with respect to which to measure the TED
	pipeline::Input<Segments> 			_goldStandard;

	// The linear constraints that describe the relationship among the segments
	pipeline::Input<LinearConstraints> 		_linearConstraints;

	// The segments that were extracted from the data
	pipeline::Input<Segments> 			_segments;

	// The reference that the IdMapCreators need to determin the size of the the output images.
	// Alternatively the size could be calculated from the segment hypotheses. 
	pipeline::Input<ImageStack>   			_reference;

	// A problem configuration to map segments to the corresponding variables.
	pipeline::Input<ProblemConfiguration> 		_problemConfiguration;
	
	/********************
	* Internal pipeline *
	********************/

	// The node that calculates the tolerant edit distance	
	boost::shared_ptr<TolerantEditDistance>		_teDistance;
	
	// The id map creator that creates image stacks for the TED from the gold standard
	boost::shared_ptr<IdMapCreator>			_gsimCreator;

	// The id map creator that creates images stacks for the TED from the reconstruction
	boost::shared_ptr<IdMapCreator>			_rimCreator;

	// A neuron extractor to convert the solution of the reconstructor into component trees	
	boost::shared_ptr<NeuronExtractor> 		_rNeuronExtractor; 

	// A neuron extractor to convert the gold standard segments into component trees
	boost::shared_ptr<NeuronExtractor>		_gsNeuronExtractor;

	// The process node that reconstructs the reconstruction solution
	boost::shared_ptr<Reconstructor>              	_rReconstructor;

	// Linear solver to find the closest reconstruction to the goldstandard with one
	// of the segments fliped that still adheres to all the constraints. Fliped here 
	// means: if the segment is in the gold standard it is not in the reconstruction,
	// and if it is not in the gold standard then it is in the reconstruction.
	boost::shared_ptr<LinearSolver>                 _linearSolver;

	// Objective generator to generate the hamming distance objective
	boost::shared_ptr<ObjectiveGenerator>       	_objectiveGenerator;

	// Hamming cost function for the objective generator
	boost::shared_ptr<HammingCostFunction> 		_hammingCostFunction;

};

#endif // SOPNET_MINIMAL_IMPACT_TED_WRITER_H___

