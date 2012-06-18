#ifndef PIPELINE_CLOCK_H__
#define PIPELINE_CLOCK_H__

#include <boost/thread.hpp>

#include <pipeline/all.h>
#include "Time.h"

class Clock : public pipeline::ProcessNode {

public:

	Clock();

	~Clock();

	void start();

	void stop();

private:

	void run();

	pipeline::Output<TimeStamp> _time;

	signals::Slot<pipeline::Modified> _modified;

	boost::thread _clockThread;

	bool _running;
};

#endif // PIPELINE_CLOCK_H__

