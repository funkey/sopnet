#ifndef SOPNET_EVALUATION_ERRORS_H__
#define SOPNET_EVALUATION_ERRORS_H__

class Errors {

public:

	/**
	 * Create an empty errors data structure without using a background label, 
	 * i.e., without false positives and false negatives.
	 */
	Errors();

	/**
	 * Create an empty errors data structure for the given background labels.
	 *
	 * @param gtBackgroundLabel
	 *             The background label in the ground truth.
	 *
	 * @param recBackgroundLabel
	 *             The background label in the reconstruction.
	 */
	Errors(float gtBackgroundLabel, float recBackgroundLabel);

	/**
	 * Clear the label mappings and error counts.
	 */
	void clear();

	/**
	 * Register a mapping from a ground truth label to a reconstruction label.  
	 * It is okay to register a mapping multiple times, mapping sizes will be 
	 * accumulated in this case.
	 * 
	 * @param gtLabel
	 *             The label of the ground truth region.
	 *
	 * @param recLabel
	 *             The label of the reconstruction region.
	 *
	 * @param size
	 *             The size of the region.
	 */
	void addMapping(float gtLabel, float recLabel, unsigned int size);

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

private:

	void addEntry(std::map<float, std::map<float, unsigned int> >& map, float a, float b, unsigned int v);

	void updateErrorCounts();

	// sparse representation of groundtruth to reconstruction confusion matrix
	std::map<float, std::map<float, unsigned int> > _gtLabelsByRec;
	std::map<float, std::map<float, unsigned int> > _recLabelsByGt;

	unsigned int _numSplits;
	unsigned int _numMerges;
	unsigned int _numFalsePositives;
	unsigned int _numFalseNegatives;

	bool _haveBackgroundLabel;

	float _gtBackgroundLabel;
	float _recBackgroundLabel;

	bool _dirty;
};

#endif // SOPNET_EVALUATION_ERRORS_H__

