// ARM_math.cpp - fixed-point helpers (compiled to smull + shift).
#include "ARM_math.h"

namespace com { namespace glu { namespace platform { namespace arm {

int smult16(int a, int b)
{
    long long product = (long long)a * (long long)b;
    return (int)(product >> 16);
}

} } } }
