#ifndef IMAGEPROCESSING_EXCEPTIONS_H__
#define IMAGEPROCESSING_EXCEPTIONS_H__

#include <util/exceptions.h>

/*
 * EXCEPTIONS
 */

struct ImageProcessingError : virtual Exception {};

struct InvalidOperation : virtual ImageProcessingError {};

#endif // IMAGEPROCESSING_EXCEPTIONS_H__

