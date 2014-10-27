/**
 * sopnet main file. Initializes all objects, views, and visualizers.
 */

#include <iostream>
#include <string>

#include <pipeline/Process.h>
#include <util/exceptions.h>
#include <imageprocessing/io/ImageStackDirectoryReader.h>
#include <sopnet/Sopnet.h>
#include <sopnet/evaluation/GroundTruthExtractor.h>
#include <sopnet/io/IdMapCreator.h>
#include <sopnet/io/NeuronsImageWriter.h>
#include <sopnet/neurons/NeuronExtractor.h>
#include <util/ProgramOptions.h>
#include <util/SignalHandler.h>

using std::cout;
using std::endl;
using namespace logger;

util::ProgramOption optionRaw(
		util::_long_name        = "raw",
		util::_description_text = "The name of the directory containing the raw sections.",
		util::_default_value    = "raw");

util::ProgramOption optionNeurons(
		util::_long_name        = "neurons",
		util::_description_text = "The name of the directory containing the neuron ids.",
		util::_default_value    = "groundtruth");

util::ProgramOption optionShowNeurons(
		util::_long_name        = "showNeurons",
		util::_description_text = "Show a 3D view for each neuron.");

class NeuronSelector : public pipeline::SimpleProcessNode<> {

public:

	NeuronSelector() {

		registerInput(_neurons, "neurons");
		registerInput(_selection, "selection");
		registerOutput(_neuron, "selected neuron");
	}

private:

	void updateOutputs() {

		_neuron = (*_neurons)[*_selection];
	}

	pipeline::Input<SegmentTrees> _neurons;
	pipeline::Input<unsigned int> _selection;
	pipeline::Output<SegmentTree> _neuron;
};

int main(int optionc, char** optionv) {

	try {

		/********
		 * INIT *
		 ********/

		// init command line parser
		util::ProgramOptions::init(optionc, optionv);

		// init logger
		LogManager::init();

		// init signal handler
		//util::SignalHandler::init();

		LOG_USER(out) << "[main] starting..." << std::endl;

		/*********
		 * SETUP *
		 *********/

		// create section readers
		pipeline::Process<ImageStackDirectoryReader> groundTruthReader(optionNeurons.as<std::string>());

		// create ground-truth extractor
		pipeline::Process<GroundTruthExtractor> groundTruthExtractor;
		groundTruthExtractor->setInput(groundTruthReader->getOutput());

		// create neurons extractor
		pipeline::Process<NeuronExtractor> neuronsExtractor;
		neuronsExtractor->setInput(groundTruthExtractor->getOutput());

		// create an id map creator that stores skeletons
		pipeline::Process<IdMapCreator> idMapCreator;
		idMapCreator->setInput("neurons", neuronsExtractor->getOutput());
		idMapCreator->setInput("reference", groundTruthReader->getOutput());

		// save the result
		pipeline::Process<NeuronsImageWriter> skeletonWriter("skeletons", "skeletons");
		skeletonWriter->setInput(idMapCreator->getOutput());
		skeletonWriter->write();

		LOG_USER(out) << "[main] exiting..." << std::endl;

	} catch (Exception& e) {

		handleException(e, std::cerr);
	}
}
