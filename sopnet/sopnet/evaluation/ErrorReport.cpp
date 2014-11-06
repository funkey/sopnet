#include "ErrorReport.h"
#include <vigra/seededregiongrowing.hxx>
#include <vigra/distancetransform.hxx>
#include <imageprocessing/io/ImageStackDirectoryWriter.h>
#include <sopnet/neurons/NeuronExtractor.h>
#include <sopnet/io/IdMapCreator.h>
#include <util/ProgramOptions.h>
#include <util/Logger.h>

util::ProgramOption optionReportVoi(
		util::_module           = "sopnet.evaluation",
		util::_long_name        = "reportVoi",
		util::_description_text = "Compute variation of information for the error report.");

util::ProgramOption optionReportRand(
		util::_module           = "sopnet.evaluation",
		util::_long_name        = "reportRand",
		util::_description_text = "Compute the RAND index for the error report.");

util::ProgramOption optionReportTed(
		util::_module           = "sopnet.evaluation",
		util::_long_name        = "reportTed",
		util::_description_text = "Compute the tolerant edit distance for the error report.");

util::ProgramOption optionReportAed(
		util::_module           = "sopnet.evaluation",
		util::_long_name        = "reportAed",
		util::_description_text = "Compute the anisotropic edit distance for the error report.");

util::ProgramOption optionReportHamming(
		util::_module           = "sopnet.evaluation",
		util::_long_name        = "reportHamming",
		util::_description_text = "Compute the Hamming distance to the gold-standard for the error report.");

util::ProgramOption optionGrowSlices(
		util::_module           = "sopnet.evaluation",
		util::_long_name        = "growSlices",
		util::_description_text = "For the computation of VOI and RAND, grow the reconstruction slices until no background label is present anymore.");

logger::LogChannel errorreportlog("errorreportlog", "[ErrorReport] ");

ErrorReport::ErrorReport() :
		_pipelineSetup(false) {

	registerInput(_groundTruthIdMap, "ground truth");
	registerInput(_groundTruth, "ground truth segments");
	registerInput(_goldStandard, "gold standard segments");
	registerInput(_reconstruction, "reconstruction segments");

	if (optionReportVoi) {

		_reportAssembler->addInput("errors", _voi->getOutput("errors"));
		registerOutput(_voi->getOutput("errors"), "voi errors");
	}

	if (optionReportRand) {

		_reportAssembler->addInput("errors", _rand->getOutput("errors"));
		registerOutput(_rand->getOutput("errors"), "rand errors");
	}

	if (optionReportTed) {

		_reportAssembler->addInput("errors", _ted->getOutput("errors"));
		registerOutput(_ted->getOutput("errors"), "ted errors");
	}

	if (optionReportAed) {

		_reportAssembler->addInput("errors", _aed->getOutput("errors"));
		registerOutput(_aed->getOutput("errors"), "aed errors");
	}

	if (optionReportHamming) {

		_reportAssembler->addInput("errors", _hamming->getOutput("errors"));
		registerOutput(_hamming->getOutput("errors"), "hamming errors");
	}

	registerOutput(_reportAssembler->getOutput("error report header"), "error report header");
	registerOutput(_reportAssembler->getOutput("error report"), "error report");
	registerOutput(_reportAssembler->getOutput("human readable error report"), "human readable error report");
}

void
ErrorReport::updateOutputs() {

	if (_pipelineSetup)
		return;

	LOG_DEBUG(errorreportlog) << "setting up internal pipeline" << std::endl;

	pipeline::Process<NeuronExtractor> neuronExtractor;
	pipeline::Process<IdMapCreator>    idMapCreator;

	pipeline::Process<> voiRandIdMapProvider;

	if (optionGrowSlices) {

		voiRandIdMapProvider = pipeline::Process<GrowSlices>();
		voiRandIdMapProvider->setInput(idMapCreator->getOutput());

	} else {

		voiRandIdMapProvider = idMapCreator;
	}

	neuronExtractor->setInput(_reconstruction);
	idMapCreator->setInput("neurons", neuronExtractor->getOutput());
	idMapCreator->setInput("reference", _groundTruthIdMap);

	_voi->setInput("stack 1", _groundTruthIdMap);
	_voi->setInput("stack 2", voiRandIdMapProvider->getOutput());

	_rand->setInput("stack 1", _groundTruthIdMap);
	_rand->setInput("stack 2", voiRandIdMapProvider->getOutput());

	_ted->setInput("ground truth", _groundTruthIdMap);
	_ted->setInput("reconstruction", idMapCreator->getOutput());

	_aed->setInput("ground truth", _groundTruth);
	_aed->setInput("result", _reconstruction);

	_hamming->setInput("gold standard", _goldStandard);
	_hamming->setInput("reconstruction", _reconstruction);

	_pipelineSetup = true;

	LOG_DEBUG(errorreportlog) << "internal pipeline set up" << std::endl;
}

void
ErrorReport::GrowSlices::updateOutputs() {

	if (!_grown)
		_grown = new ImageStack();
	else
		_grown->clear();

	foreach (boost::shared_ptr<Image> image, *_stack) {

		boost::shared_ptr<Image> grown = boost::make_shared<Image>(image->width(), image->height());
		*grown = *image;

		// perform Euclidean distance transform
		vigra::MultiArray<2, float> dist(image->width(), image->height());
		vigra::distanceTransform(*image, dist, 0, 2);

		float min, max;
		image->minmax(&min, &max);

		// init statistics functor
		vigra::ArrayOfRegionStatistics<vigra::SeedRgDirectValueFunctor<float> > stats(max);

		// grow regions
		vigra::seededRegionGrowing(dist, *grown, *grown, stats);

		_grown->add(grown);
	}

	pipeline::Process<ImageStackDirectoryWriter> writer("result_grown");
	writer->setInput(_grown.getSharedPointer());
	writer->write();
}
