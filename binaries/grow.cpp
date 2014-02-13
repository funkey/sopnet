/**
 * grow main file. Initializes all objects, views, and visualizers.
 *
 * Dislocates the boundaries of found neurons by growing.
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
		util::_description_text = "The amount in nm by which to grow.",
		util::_default_value    = 50);

util::ProgramOption optionHeadless(
		util::_long_name        = "headless",
		util::_description_text = "Don't show the gui.");

class Grow : public pipeline::SimpleProcessNode<> {

public:

	Grow() {

		registerInput(_stack, "image stack");
		registerOutput(_grown, "grown");
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

		_grown->clear();
		for (int i = 0; i < depth; i++) {
			_grown->add(boost::make_shared<Image>(width, height, 0.5));
		}

		for (int z = 0; z < depth; z++)
			for (int y = 0; y < height; y++)
				for (int x = 0; x < width; x++) {

					float k = (*(*_stack)[z])(x, y);
					if (k != 0) {

						(*(*_grown)[z])(x, y) = k;
						continue;
					}

					std::map<float, unsigned int> counts;

					for (int dz = -distanceZ; dz <= distanceZ; dz++)
						for (int dy = -distanceY; dy < distanceY; dy++)
							for (int dx = -distanceX; dx < distanceY; dx++) {

								// on sphere?
								if (dx*dx*resX*resX + dy*dy*resY*resY + dz*dz*resZ*resZ <= distance*distance)
									continue;

								int tx = x + dx;
								int ty = y + dy;
								int tz = z + dz;

								// in volume
								if (tx < 0 || tx >= width || ty < 0 || ty >= height || tz < 0 || tz >= depth)
									// assume background label at outside
									counts[0]++;
								else
									counts[(*(*_stack)[tz])(tx, ty)]++;
							}

					unsigned int maxCount = 0;
					float maxLabel = 0;
					unsigned int count;
					float label;
					foreach (boost::tie(label, count), counts)
						if (count > maxCount) {
							maxCount = count;
							maxLabel = label;
						}

					(*(*_grown)[z])(x, y) = maxLabel;
				}
	}

	pipeline::Input<ImageStack>  _stack;
	pipeline::Output<ImageStack> _grown;
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

		// grow process node

		pipeline::Process<Grow> grow;

		// connect

		grow->setInput(groundTruthReader->getOutput());

		if (!optionHeadless) {

			// start GUI

			pipeline::Process<ImageStackView> gtView;
			pipeline::Process<ImageStackView> growView;
			pipeline::Process<gui::ZoomView>  zoomView;
			pipeline::Process<gui::Window>    window("edit distance");

			gtView->setInput(groundTruthReader->getOutput());
			growView->setInput(grow->getOutput());

			pipeline::Process<gui::NamedView> gtNamedView("ground truth");
			pipeline::Process<gui::NamedView> growNamedView("grow");

			gtNamedView->setInput(gtView->getOutput());
			growNamedView->setInput(growView->getOutput());

			pipeline::Process<gui::ContainerView<gui::HorizontalPlacing> > container;
			container->setSpacing(10);
			container->setAlign(gui::HorizontalPlacing::Bottom);
			container->addInput(gtNamedView->getOutput());
			container->addInput(growNamedView->getOutput());

			zoomView->setInput(container->getOutput());
			window->setInput(zoomView->getOutput());

			window->processEvents();
		}

		// save results

		pipeline::Process<ImageStackDirectoryWriter> growWriter("groundtruth_grown");
		growWriter->setInput(grow->getOutput());
		growWriter->write();

	} catch (Exception& e) {

		handleException(e, std::cerr);
	}
}

