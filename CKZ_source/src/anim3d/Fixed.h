// Fixed.h - 20.12 fixed point helpers of the animation module (created by b047x).
// The original tree has both "Fixed.h" and "fixed.h" in includes (case-insensitive file system);
// this is the one file.
#ifndef ANIM3D_FIXED_H
#define ANIM3D_FIXED_H

class Fixed {
public:
    /** 1 / 4096: fixed -> float factor. */
    static const float FIXED_TO_FLOAT_COEF;
    /** sin(0..90 degrees) in 20.12. */
    static const int SIN_COS_TABLE[91];

    /** a * b in 20.12, split so that no intermediate overflows. */
    static inline int mul(int a, int b)
    {
        return (a >> 12) * b + (b >> 12) * (a & 0xfff) + (((a & 0xfff) * (b & 0xfff)) >> 12);
    }

    /** a / b in 20.12 (64 bit intermediate). */
    static inline int div(int a, int b)
    {
        return (int)(((long long)a << 12) / b);
    }

    /** Integer part, rounded towards zero. */
    static inline int toInt(int value)
    {
        if (value < 0) {
            return -((-value) >> 12);
        }
        return value >> 12;
    }

    static inline float toFloat(int value)
    {
        return (float)value * FIXED_TO_FLOAT_COEF;
    }

    static inline int fromFloat(float value)
    {
        return (int)(value * 4096.0f);
    }

    /** Square root of a 20.12 value (8 Newton steps). */
    static int sqrt(int value);
    /** Sine of `angle` (degrees, 20.12), interpolated between whole degrees. */
    static int sin(int angle);
};

#endif
