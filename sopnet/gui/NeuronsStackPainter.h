#ifndef SOPNET_GUI_NEURONS_STACK_PAINTER_H__
#define SOPNET_GUI_NEURONS_STACK_PAINTER_H__

#include <set>

#include <boost/array.hpp>
#include <boost/shared_ptr.hpp>

#include <gui/Painter.h>
#include <sopnet/neurons/Neurons.h>
#include "SliceTextures.h"

class NeuronsStackPainter : public gui::Painter {

public:

	NeuronsStackPainter();

	/**
	 * Set a new set of segments.
	 */
	void setNeurons(boost::shared_ptr<Neurons> segments);

	/**
	 * Change the currently visible section.
	 */
	void setCurrentSection(unsigned int section);

	/**
	 * Show only a single neuron.
	 */
	void showNeuron(unsigned int neuron);

	/**
	 * Show all neurons.
	 */
	void showAllNeurons();

	/**
	 * Show end segments.
	 */
	void showEnds(bool show);

	/**
	 * Show continuation segments.
	 */
	void showContinuations(bool show);

	/**
	 * Show branch segments.
	 */
	void showBranches(bool show);

	/**
	 * Show the ids of the slices and their connection partners.
	 */
	void showSliceIds(bool show);

	/**
	 * Overwritten from painter.
	 */
	virtual void draw(
		const util::rect<double>&  roi,
		const util::point<double>& resolution);

private:

	// add the given slice's bounding box to the current size
	util::rect<double> sizeAddSlice(const util::rect<double>& currentSize, const Slice& slice);

	// find a random color for each neuron
	void assignColors();

	// load all slice textures
	void loadTextures();

	void drawNeuron(
		Neuron& neuron,
		unsigned int neuronNum,
		const util::rect<double>&  roi,
		const util::point<double>& resolution);

	void drawSlice(
			const Slice& slice,
			double red, double green, double blue,
			double alpha,
			const util::rect<double>&  roi,
			const util::point<double>& resolution);

	void drawMerge(
			const util::point<double>& source1,
			const util::point<double>& source2,
			const util::point<double>& target,
			Direction direction,
			const util::rect<double>& roi,
			const util::point<double>& resolution);

	void drawBranch(
			const util::point<double>& source,
			const util::point<double>& target1,
			const util::point<double>& target2,
			Direction direction,
			const util::rect<double>& roi,
			const util::point<double>& resolution);

	void drawContinuation(
			const util::point<double>& source,
			const util::point<double>& target,
			Direction direction,
			const util::rect<double>& roi,
			const util::point<double>& resolution);

	void drawEnd(
			const util::point<double>& center,
			Direction direction,
			const util::rect<double>& roi,
			const util::point<double>& resolution);

	// set the color to draw links to next section
	void setNextColor();

	// set the color to draw links to prev section
	void setPrevColor();

	// the neurons
	boost::shared_ptr<Neurons> _neurons;

	// the textures of the slices to draw
	SliceTextures _textures;

	// the section to draw
	unsigned int _section;

	// whether to show a single neuron only
	bool _showSingleNeuron;

	// the currently selected neuron
	unsigned int _currentNeuron;

	// show certain types of segments
	bool _showEnds;
	bool _showContinuations;
	bool _showBranches;

	// show slice ids
	bool _showSliceIds;

	// distance between sections
	double _zScale;

	// a lookup table for neuron colors
	std::map<unsigned int, boost::array<double, 3> > _colors;
};

#endif // SOPNET_GUI_NEURONS_STACK_PAINTER_H__

