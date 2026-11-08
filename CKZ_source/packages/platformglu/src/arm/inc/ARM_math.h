// ARM_math.h - small fixed-point helpers that map onto single ARM instructions.
#ifndef ARM_MATH_H
#define ARM_MATH_H

namespace com { namespace glu { namespace platform { namespace arm {

/** 16.16 fixed-point multiply: (a * b) >> 16 with a 64-bit intermediate. */
int smult16(int a, int b);

} } } }

#endif
