#ifndef SOPNET_EVALUATION_TOLERANT_EDIT_DISTANCE_ERRORS_H__
#define SOPNET_EVALUATION_TOLERANT_EDIT_DISTANCE_ERRORS_H__

#include "Cell.h"
#include "Errors.h"

/**
 * Representation of split and merge (and optionally false positive and false 
 * negative) errors between a ground truth and a reconstruction. All errors are 
 * evaluated based on a mapping of Cells from a ground truth label to a 
 * reconstruction label. A cell represents a set of image locations and is in 
 * this context the atomic unit to be labelled.
 */
class TolerantEditDistanceErrors : public Errors {

public:

	typedef Cell<float>                                                cell_t;
	typedef boost::shared_ptr<std::vector<cell_t> >                    cells_t;
	typedef std::map<float, std::map<float, std::set<unsigned int> > > cell_map_t;

	/**
	 * Create an empty errors data structure without using a background label, 
	 * i.e., without false positives and false negatives.
	 */
	TolerantEditDistanceErrors();

	/**
	 * Create an empty errors data structure for the given background labels.
	 *
	 * @param gtBackgroundLabel
	 *             The background label in the ground truth.
	 *
	 * @param recBackgroundLabel
	 *             The background label in the reconstruction.
	 */
	TolerantEditDistanceErrors(float gtBackgroundLabel, float recBackgroundLabel);

	/**
	 * Set the list of cells this errors data structure is working on. This has 
	 * to be done before calling addMapping() or getOverlap().
	 *
	 * @param cells
	 *             A list of cells (sets of image locations) that partitions the 
	 *             ground truth and reconstruction volumes. Each cell has a 
	 *             ground truth label and can be mapped via addMapping() to an 
	 *             arbitrary reconstruction label.
	 */
	void setCells(cells_t cells);

	/**
	 * Clear the label mappings and error counts.
	 */
	void clear();

	/**
	 * Register a mapping from a cell to a reconstruction label.
	 * 
	 * @param cellIndex
	 *             The index of the cell in the cell list.
	 *
	 * @param recLabel
	 *             The reconstruction label of the cell.
	 */
	void addMapping(unsigned int cellIndex, float recLabel);

	/**
	 * Get all reconstruction labels that map to the given ground truth label.
	 */
	std::vector<float> getReconstructionLabels(float gtLabel);

	/**
	 * Get all ground truth labels that map to the given reconstruction label.
	 */
	std::vector<float> getGroundTruthLabels(float recLabel);

	/**
	 * Get the number of locations shared by the given ground truth and 
	 * reconstruction label.
	 */
	unsigned int getOverlap(float gtLabel, float recLabel);

	unsigned int getNumSplits();
	unsigned int getNumMerges();
	unsigned int getNumFalsePositives();
	unsigned int getNumFalseNegatives();

	/**
	 * Get the sum of all errors.
	 */
	unsigned int getNumErrors() {

		return getNumSplits() + getNumMerges() + getNumFalsePositives() + getNumFalseNegatives();
	}

	/**
	 * Get all ground truth labels that got split in the reconstruction.
	 */
	std::set<float> getSplitLabels();

	/**
	 * Get all reconstruction labels that merge multiple ground truh labels.
	 */
	std::set<float> getMergeLabels();

	/**
	 * Get all reconstruction labels that split the given ground truth label.
	 */
	std::set<float> getSplits(float gtLabel);

	/**
	 * Get all ground truth labels that the given reconstruction label merges.
	 */
	std::set<float> getMerges(float recLabel);

	/**
	 * Get all cells that split the given ground truth label.
	 */
	const cell_map_t::mapped_type& getSplitCells(float gtLabel);

	/**
	 * Get all cells that the given reconstruction label merges.
	 */
	const cell_map_t::mapped_type& getMergeCells(float recLabel);

	/**
	 * Get all cells that are false positives.
	 */
	const cell_map_t::mapped_type& getFalsePositiveCells();

	/**
	 * Get all cells that are false negatives.
	 */
	const cell_map_t::mapped_type& getFalseNegativeCells();

	std::string errorHeader() { return "TED_FP\tTED_FN\tTED_FS\tTED_FM\tTED_SUM"; }

	std::string errorString() {

		std::stringstream ss;
		ss
				<< getNumFalsePositives() << "\t"
				<< getNumFalseNegatives() << "\t"
				<< getNumSplits() << "\t"
				<< getNumMerges() << "\t"
				<< getNumErrors();

		return ss.str();
	}

	std::string humanReadableErrorString() {

		std::stringstream ss;
		ss
				<<   "TED FP: " << getNumFalsePositives()
				<< ", TED FN: " << getNumFalseNegatives()
				<< ", TED FS: " << getNumSplits()
				<< ", TED FM: " << getNumMerges()
				<< ", TED Total: " << getNumErrors();

		return ss.str();
	}

private:

	void addEntry(cell_map_t& map, float a, float b, unsigned int v);

	void updateErrorCounts();

	void findSplits(
			const cell_map_t& cellMap,
			cell_map_t&       splits,
			unsigned int&     numSplits,
			unsigned int&     numFalsePositives,
			float             backgroundLabel);

	// a list of cells partitioning the image
	cells_t _cells;

	// sparse representation of groundtruth to reconstruction confusion matrix
	cell_map_t _cellsByRecToGtLabel;
	cell_map_t _cellsByGtToRecLabel;

	// subset of the confusion matrix without one-to-one mappings
	cell_map_t _splits;
	cell_map_t _merges;

	unsigned int _numSplits;
	unsigned int _numMerges;
	unsigned int _numFalsePositives;
	unsigned int _numFalseNegatives;

	bool _haveBackgroundLabel;

	float _gtBackgroundLabel;
	float _recBackgroundLabel;

	bool _dirty;
};

#endif // SOPNET_EVALUATION_TOLERANT_EDIT_DISTANCE_ERRORS_H__

