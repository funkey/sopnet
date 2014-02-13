/**
 * watershed main file. Initializes all objects, views, and visualizers.
 */

#include <iostream>
#include <string>
#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>
#include <boost/progress.hpp>
#include <boost/tuple/tuple.hpp> // for vigra watersheds

#include <vigra/multi_watersheds.hxx>
#include <vigra/transformimage.hxx>

#include <util/exceptions.h>
#include <gui/ContainerView.h>
#include <gui/HorizontalPlacing.h>
#include <gui/NamedView.h>
#include <gui/RotateView.h>
#include <gui/Window.h>
#include <gui/ZoomView.h>
#include <imageprocessing/GraphCut.h>
#include <imageprocessing/GraphCutSequence.h>
#include <imageprocessing/ImageExtractor.h>
#include <imageprocessing/SubStackSelector.h>
#include <imageprocessing/gui/ImageView.h>
#include <imageprocessing/gui/ImageStackView.h>
#include <imageprocessing/gui/GraphCutDialog.h>
#include <imageprocessing/io/ImageStackHdf5Reader.h>
#include <imageprocessing/io/ImageStackDirectoryReader.h>
#include <pipeline/Process.h>
#include <util/ProgramOptions.h>

using std::cout;
using std::endl;
using namespace gui;
using namespace logger;

class Watershed : public pipeline::SimpleProcessNode<> {

public:

	Watershed() {

		registerInput(_image, "image");
		registerInput(_threshold, "seed threshold");
		registerOutput(_watersheds, "watersheds");
	}

private:

	void updateOutputs() {

		// adjust the size of the segmentation output to match the input image
		_watersheds->reshape(_image->shape());

		unsigned int labels = vigra::watershedsMultiArray(
				*_image, *_watersheds,
				vigra::DirectNeighborhood,
				vigra::WatershedOptions()
						.seedOptions(vigra::SeedOptions()
								.minima()
								.threshold(*_threshold)));

		vigra::transformImage(
				*_watersheds,
				*_watersheds,
				vigra::linearIntensityTransform<float>(1.0 / labels));
	}

	pipeline::Input<Image>  _image;
	pipeline::Input<float>  _threshold;
	pipeline::Output<Image> _watersheds;
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

		// create a window
		pipeline::Process<gui::Window> window("sopnet");

		// create a zoom view for this window
		pipeline::Process<gui::ZoomView> zoomView;
		window->setInput(zoomView->getOutput());

		// create main container view
		pipeline::Process<ContainerView<VerticalPlacing> > mainContainer;

		// create image views
		pipeline::Process<ImageStackView> membranesView;

		// create threshold slider
		pipeline::Process<Slider<float> > thresholdSlider("seed threshold", 0.0f, 1.0f, 0.1f);

		// connect them to the window via the zoom view
		mainContainer->addInput(membranesView->getOutput("painter"));
		mainContainer->addInput(thresholdSlider->getOutput("painter"));
		zoomView->setInput(mainContainer->getOutput());

		// create section readers
		pipeline::Process<ImageStackDirectoryReader> membranesReader("./membranes/");

		// set input for image stack views
		membranesView->setInput(membranesReader->getOutput());

		// create watershed node
		pipeline::Process<Watershed> watershed;

		// set input to watershed
		watershed->setInput("image", membranesView->getOutput("current image"));
		watershed->setInput("seed threshold", thresholdSlider->getOutput("value"));

		// create image view for watershed result
		pipeline::Process<ImageView> segmentationView;

		// connect it to watershed
		segmentationView->setInput(watershed->getOutput("watersheds"));

		// add it to main container
		mainContainer->addInput(segmentationView->getOutput("painter"));

		while (!window->closed()) {

			window->processEvents();
			usleep(1000);
		}

		LOG_USER(out) << "[main] exiting..." << std::endl;

	} catch (Exception& e) {

		handleException(e, std::cerr);
	}
}
