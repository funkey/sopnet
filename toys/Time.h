#ifndef TIME_H__
#define TIME_H__

#include <pipeline.h>

struct TimeStamp : public pipeline::Data {

	TimeStamp() :
		seconds(0) {}

	TimeStamp(unsigned int seconds_) :
		seconds(seconds_) {}

	int seconds;
};

#endif // TIME_H__

