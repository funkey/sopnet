#ifndef SOPNET_EVALUATION_ERROR_REPORT_H__
#define SOPNET_EVALUATION_ERROR_REPORT_H__

#include <string>
#include <pipeline/SimpleProcessNode.h>
#include <imageprocessing/ImageStack.h>
#include <sopnet/segments/Segments.h>
#include "VariationOfInformation.h"
#include "RandIndex.h"
#include "AnisotropicEditDistance.h"
#include "TolerantEditDistance.h"
#include "HammingDistance.h"

class ErrorReport : public pipeline::SimpleProcessNode<> {

public:

	ErrorReport();

private:

	class ReportAssembler : public pipeline::SimpleProcessNode<> {

	public:

		ReportAssembler() {

			registerInputs(_errors, "errors");
			registerOutput(_reportHeader, "error report header");
			registerOutput(_report, "error report");
			registerOutput(_humanReadableReport, "human readable error report");
		}

	private:

		void updateOutputs() {

			_reportHeader        = new std::string();
			_report              = new std::string();
			_humanReadableReport = new std::string();

			foreach (const pipeline::Input<Errors>& errors, _errors) {

				if (!_reportHeader->empty())
					(*_reportHeader) += "\t";

				if (!_report->empty())
					(*_report) += "\t";

				if (!_humanReadableReport->empty())
					(*_humanReadableReport) += "; ";

				(*_reportHeader)        += errors->errorHeader();
				(*_report)              += errors->errorString();
				(*_humanReadableReport) += errors->humanReadableErrorString();
			}
		}

		pipeline::Inputs<Errors> _errors;
		pipeline::Output<std::string> _reportHeader;
		pipeline::Output<std::string> _report;
		pipeline::Output<std::string> _humanReadableReport;
	};

	void updateOutputs();

	pipeline::Input<ImageStack> _groundTruthIdMap;
	pipeline::Input<Segments>   _groundTruth;
	pipeline::Input<Segments>   _goldStandard;
	pipeline::Input<Segments>   _reconstruction;

	pipeline::Process<VariationOfInformation>  _voi;
	pipeline::Process<RandIndex>               _rand;
	pipeline::Process<AnisotropicEditDistance> _aed;
	pipeline::Process<TolerantEditDistance>    _ted;
	pipeline::Process<HammingDistance>         _hamming;
	pipeline::Process<ReportAssembler>         _reportAssembler;

	pipeline::Output<VariationOfInformationErrors>  _voiErrors;
	pipeline::Output<RandIndexErrors>               _randErrors;
	pipeline::Output<AnisotropicEditDistanceErrors> _aedErrors;
	pipeline::Output<TolerantEditDistanceErrors>    _tedErrors;
	pipeline::Output<HammingDistanceErrors>         _hammingErrors;
	pipeline::Output<std::string>                   _reportLine;
	pipeline::Output<std::string>                   _humanReadableReport;

	bool _pipelineSetup;
};

#endif // SOPNET_EVALUATION_ERROR_REPORT_H__

