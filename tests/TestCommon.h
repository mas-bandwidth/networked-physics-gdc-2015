#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include "Common.h"

using namespace protocol;

void sleep_after_too_many_iterations( int & iteration, int sleep_after = 256 )
{
    iteration++;
    if ( iteration > sleep_after )
        sleep_milliseconds(1);
}

#endif
