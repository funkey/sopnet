#include "Clock.h"

Clock::Clock() :
	_time(new TimeStamp(0)),
	_running(false) {

	registerOutput(_time, "time");

	_time.registerForwardSlot(_modified);
}

Clock::~Clock() {

	// stop the clock thread, if it was running
	stop();
}

void
Clock::start() {

	_running = true;

	_clockThread = boost::thread(boost::bind(&Clock::run, this));
}

void
Clock::stop() {

	if (!_running)
		return;

	_running = false;

	_clockThread.join();
}

void
Clock::run() {

	while (_running) {

		usleep(10000);

		_time->seconds++;

		_modified();
	}
}
