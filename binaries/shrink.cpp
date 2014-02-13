/**
 * shrink main file. Initializes all objects, views, and visualizers.
 *
 * Dislocates the boundaries of found neurons by shrinking.
 */

#include <iostream>
#include <vigra/multi_distance.hxx>
#include <gui/ContainerView.h>
#include <gui/HorizontalPlacing.h>
#include <gui/NamedView.h>
#include <gui/Window.h>
#include <gui/ZoomView.h>
#include <imageprocessing/gui/ImageStackView.h>
#include <imageprocessing/io/ImageStackDirectoryReader.h>
#include <imageprocessing/io/ImageStackDirectoryWriter.h>
#include <pipeline/Process.h>
#include <pipeline/Value.h>
#include <util/ProgramOptions.h>
#include <util/Logger.h>

using namespace logger;

util::ProgramOption optionGroundTruth(
		util::_long_name        = "groundTruth",
		util::_description_text = "The ground truth image stack.",
		util::_default_value    = "groundtruth");

util::ProgramOption optionDistance(
		util::_long_name        = "distance",
		util::_description_text = "The amount in nm by which to shrink.",
		util::_default_value    = 50);

util::ProgramOption optionKeep(
		util::_long_name        = "keep",
		util::_description_text = "The minimal size of each region to keep while shrinking.",
		util::_default_value    = 10);

util::ProgramOption optionHeadless(
		util::_long_name        = "headless",
		util::_description_text = "Don't show the gui.");

class Grow : public pipeline::SimpleProcessNode<> {

public:

	Grow() {

		registerInput(_stack, "image stack");
		registerOutput(_shrunken, "shrunken");
	}

private:

	void updateOutputs() {

		float resX = 4.0;
		float resY = 4.0;
		float resZ = 40.0;

		float shrinkDistance2 = optionDistance.as<float>()*optionDistance.as<float>();
		float keepDistance = optionKeep;

		int depth = _stack->size();
		if (depth == 0)
			return;
		int width  = (*_stack)[0]->width();
		int height = (*_stack)[0]->height();

		vigra::MultiArray<3, float> boundaryDistance2(vigra::Shape3(width, height, depth));
		boundaryDistance2 = 0;

		for (int z = 0; z < depth; z++)
			for (int y = 0; y < height; y++)
				for (int x = 0; x < width; x++)
					if ((*(*_stack)[z])(x, y) == 0)
						boundaryDistance2(x, y, z) = 1.0;


		float pitch[3];
		pitch[0] = resX;
		pitch[1] = resY;
		pitch[2] = resZ;

		// compute l2 distance for each pixel to boundary
		vigra::separableMultiDistSquared(
				boundaryDistance2,
				boundaryDistance2,
				true /* background */,
				pitch);

		// for each region, get maximal boundary distance
		std::map<float, float> maxDistances2;
		for (int z = 0; z < depth; z++)
			for (int y = 0; y < height; y++)
				for (int x = 0; x < width; x++) {

					float label = (*(*_stack)[z])(x, y);
					maxDistances2[label] = std::max(boundaryDistance2(x, y, z), maxDistances2[label]);
				}

		// prepare output stack
		_shrunken->clear();
		for (int i = 0; i < depth; i++) {
			_shrunken->add(boost::make_shared<Image>(width, height, 0.5));
		}

		// shrink
		for (int z = 0; z < depth; z++)
			for (int y = 0; y < height; y++)
				for (int x = 0; x < width; x++) {

					float label = (*(*_stack)[z])(x, y);

					if (label == 0) {

						(*(*_shrunken)[z])(x, y) = 0;
						continue;
					}

					float maxDistance2 = maxDistances2[label];
					float distance2 = boundaryDistance2(x, y, z);

					if (distance2 <= shrinkDistance2 && (sqrt(maxDistance2) - sqrt(distance2)) > keepDistance)
						(*(*_shrunken)[z])(x, y) = 0;
					else
						(*(*_shrunken)[z])(x, y) = label;
				}
	}

	pipeline::Input<ImageStack>  _stack;
	pipeline::Output<ImageStack> _shrunken;
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

		LOG_USER(out) << "[main] starting..." << std::endl;

		/*********
		 * SETUP *
		 *********/

		// setup file readers and writers

		pipeline::Process<ImageStackDirectoryReader> groundTruthReader(optionGroundTruth.as<std::string>());

		// shrink process node

		pipeline::Process<Grow> shrink;

		// connect

		shrink->setInput(groundTruthReader->getOutput());

		if (!optionHeadless) {

			// start GUI

			pipeline::Process<ImageStackView> gtView;
			pipeline::Process<ImageStackView> shrinkView;
			pipeline::Process<gui::ZoomView>  zoomView;
			pipeline::Process<gui::Window>    window("edit distance");

			gtView->setInput(groundTruthReader->getOutput());
			shrinkView->setInput(shrink->getOutput());

			pipeline::Process<gui::NamedView> gtNamedView("ground truth");
			pipeline::Process<gui::NamedView> shrinkNamedView("shrink");

			gtNamedView->setInput(gtView->getOutput());
			shrinkNamedView->setInput(shrinkView->getOutput());

			pipeline::Process<gui::ContainerView<gui::HorizontalPlacing> > container;
			container->setSpacing(10);
			container->setAlign(gui::HorizontalPlacing::Bottom);
			container->addInput(gtNamedView->getOutput());
			container->addInput(shrinkNamedView->getOutput());

			zoomView->setInput(container->getOutput());
			window->setInput(zoomView->getOutput());

			window->processEvents();
		}

		// save results

		pipeline::Process<ImageStackDirectoryWriter> shrinkWriter("groundtruth_shrunken");
		shrinkWriter->setInput(shrink->getOutput());
		shrinkWriter->write();

	} catch (Exception& e) {

		handleException(e, std::cerr);
	}
}


