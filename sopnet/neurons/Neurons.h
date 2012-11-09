#ifndef SOPNET_NEURONS_NEURONS_H__
#define SOPNET_NEURONS_NEURONS_H__

#include <vector>

#include <boost/shared_ptr.hpp>

#include <pipeline/all.h>
#include "Neuron.h"

class Neurons : public pipeline::Data {

	typedef std::vector<boost::shared_ptr<Neuron> > neurons_type;

public:

	typedef neurons_type::const_iterator            const_iterator;
	typedef neurons_type::iterator                  iterator;

	/**
	 * Remove all neurons.
	 */
	void clear() { _neurons.clear(); }

	/**
	 * Add a neuron.
	 */
	void add(boost::shared_ptr<Neuron> neuron) { _neurons.push_back(neuron); }

	/**
	 * Add a set of neurons.
	 */
	void addAll(boost::shared_ptr<Neurons> neurons) { foreach (boost::shared_ptr<Neuron> neuron, *neurons) _neurons.push_back(neuron); }

	/**
	 * Get the number of neurons.
	 */
	unsigned int size() { return _neurons.size(); }

	const const_iterator begin() const { return _neurons.begin(); }

	iterator begin() { return _neurons.begin(); }

	const const_iterator end() const { return _neurons.end(); }

	iterator end() { return _neurons.end(); }

	/**
	 * Get the number of sections these neurons cover.
	 */
	unsigned int getNumSections();

private:

	neurons_type _neurons;
};

#endif // SOPNET_NEURONS_NEURONS_H__

