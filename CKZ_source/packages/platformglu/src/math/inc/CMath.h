// CMath.h - floating-point constants and small generic helpers of the Glu math module.
#ifndef CMATH_H
#define CMATH_H

namespace com { namespace glu { namespace platform { namespace math {

class CMath {
public:
    static const float PI;      // 3.14159...

    /** Smaller of two values. */
    template <class T>
    static T Min(T a, T b)
    {
        if (a < b) {
            return a;
        }
        return b;
    }

    /** Larger of two values. */
    template <class T>
    static T Max(T a, T b)
    {
        if (a < b) {
            return b;
        }
        return a;
    }
};

} } } }

#endif
