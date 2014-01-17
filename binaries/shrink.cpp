/**
 * shrink main file. Initializes all objects, views, and visualizers.
 *
 * Dislocates the boundaries of found neurons by shrinking.
 */

#include <iostream>
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

		float distance = optionDistance;

		int depth = _stack->size();
		if (depth == 0)
			return;
		int width  = (*_stack)[0]->width();
		int height = (*_stack)[0]->height();

		int distanceX = (int)optionDistance/resX;
		int distanceY = (int)optionDistance/resY;
		int distanceZ = (int)optionDistance/resZ;

		_shrunken->clear();
		for (int i = 0; i < depth; i++) {
			boost::shared_ptr<std::vector<float> > data = boost::make_shared<std::vector<float> >(width*height, 0.5);
			_shrunken->add(boost::make_shared<Image>(width, height, data));
		}

		for (int z = 0; z < depth; z++)
			for (int y = 0; y < height; y++)
				for (int x = 0; x < width; x++) {

					bool done = false;

					for (int dz = -distanceZ; dz <= distanceZ; dz++) {
						for (int dy = -distanceY; dy < distanceY; dy++) {
							for (int dx = -distanceX; dx < distanceY; dx++) {

								// on sphere?
								if (dx*dx*resX*resX + dy*dy*resY*resY + dz*dz*resZ*resZ <= distance*distance)
									continue;

								int tx = x + dx;
								int ty = y + dy;
								int tz = z + dz;

								// in volume
								if (tx < 0 || tx >= width || ty < 0 || ty >= height || tz < 0 || tz >= depth)
									continue;

								if ((*(*_stack)[tz])(tx, ty) == 0) {

									(*(*_shrunken)[z])(x, y) = 0;
									done = true;
									break;
								}
							}

							if (done)
								break;
						}

						if (done)
							break;
					}

					if (!done)
						(*(*_shrunken)[z])(x, y) = (*(*_stack)[z])(x, y);
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


