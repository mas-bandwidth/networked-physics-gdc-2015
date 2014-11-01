#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include "core/Core.h"

void sleep_after_too_many_iterations( int & iteration, int sleep_after = 256 )
{
    iteration++;
    if ( iteration > sleep_after )
        core::sleep_milliseconds(1);
}

#endif
