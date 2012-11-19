#ifndef SOPNET_EVALUATION_RESULT_EVALUATOR_H__
#define SOPNET_EVALUATION_RESULT_EVALUATOR_H__

#include <pipeline/all.h>

#include <sopnet/features/Overlap.h>
#include <sopnet/segments/Segments.h>
#include "Errors.h"

class ResultEvaluator : public pipeline::SimpleProcessNode<> {

	typedef std::vector<std::pair<int, int> > Mapping;
	typedef std::vector<Mapping>              Mappings;

public:

	/**
	 * @param minOverlap
	 *             The minimal overlap between a result slice and a ground-truth 
	 *             slice to consider them for being the same.
	 */
	ResultEvaluator(double minOverlap = 0.5);

private:

	void updateOutputs();

	void findAllSlicesAndLinks();

	std::vector<boost::shared_ptr<Slice> > getGroundTruthSlices(unsigned int section);

	std::vector<boost::shared_ptr<Slice> > getResultSlices(unsigned int section);

	std::vector<std::vector<boost::shared_ptr<Slice> > > findSlices(Segments& segments);

	std::vector<std::set<std::pair<int, int> > > findLinks(Segments& segments);

	void insertSlice(
			std::vector<std::set<boost::shared_ptr<Slice> > >& sliceSets,
			boost::shared_ptr<Slice>                           slice);

	std::vector<Mapping> getAllMappings(
			unsigned int section);

	std::vector<boost::shared_ptr<Slice> > getSlices(
			Segments& segments,
			unsigned int section);

	void createMappings(
			Mappings&                               mappings,
			Mapping&                                currentMapping,
			std::map<int, std::vector<int> >&       resultPartners,
			std::vector<boost::shared_ptr<Slice> >& resultSlices,
			unsigned int                            numSlice);

	Errors getErrors(
			const Mapping& mapping,
			const Mapping& previousMapping,
			unsigned int section);

	Errors getIntraErrors(
			const Mapping& mapping,
			unsigned int section);

	Errors getInterErrors(
			const Mapping& mapping,
			const Mapping& previousMapping,
			unsigned int section);

	pipeline::Input<Segments> _result;
	pipeline::Input<Segments> _groundTruth;

	pipeline::Output<Errors> _errors;

	unsigned int _numSections;

	Overlap _overlap;

	double _minOverlap;

	// all slices sorted by sections
	std::vector<std::vector<boost::shared_ptr<Slice> > > _resultSlices;
	std::vector<std::vector<boost::shared_ptr<Slice> > > _groundTruthSlices;
	std::vector<std::set<std::pair<int, int> > > _resultLinks;
	std::vector<std::set<std::pair<int, int> > > _groundTruthLinks;
};

#endif // SOPNET_EVALUATION_RESULT_EVALUATOR_H__

