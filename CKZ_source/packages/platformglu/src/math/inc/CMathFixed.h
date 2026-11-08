// CMathFixed.h - 16.16 fixed-point math helpers (platformglu).
#ifndef CMATHFIXED_H
#define CMATHFIXED_H

namespace com { namespace glu { namespace platform { namespace math {

/** 16.16 fixed-point math; angles are fixed-point degrees. */
class CMathFixed {
public:
    static const int FBits;         // fraction bits
    static const int One;           // 1.0
    static const int Degrees_90;    // 90 degrees

    /** Sine of a fixed-point angle in degrees, result in 16.16. */
    static int Sin(int angle);

    /** Product of two fixed-point numbers. */  // added by b002
    static int Mul(int left, int right)  // added by b002
    {  // added by b002
        return (int)(((long long)left * (long long)right) >> FBits);  // added by b002
    }  // added by b002

    /** Quotient of two fixed-point numbers. */  // added by b042
    static int Div(int numerator, int denominator)  // added by b042
    {  // added by b042
        return (int)(((long long)numerator << FBits) / denominator);  // added by b042
    }  // added by b042
};

} } } }

#endif
