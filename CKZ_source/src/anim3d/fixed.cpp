// fixed.cpp - 20.12 fixed point square root and sine (created by b047x).
#include "Fixed.h"

const float Fixed::FIXED_TO_FLOAT_COEF = 1.0f / 4096.0f;

const int Fixed::SIN_COS_TABLE[91] = {
    0, 71, 142, 214, 285, 356, 428, 499,
    570, 640, 711, 781, 851, 921, 990, 1060,
    1129, 1197, 1265, 1333, 1400, 1467, 1534, 1600,
    1665, 1731, 1795, 1859, 1922, 1985, 2048, 2109,
    2170, 2230, 2290, 2349, 2407, 2465, 2521, 2577,
    2632, 2687, 2740, 2793, 2845, 2896, 2946, 2995,
    3043, 3091, 3137, 3183, 3227, 3271, 3313, 3355,
    3395, 3435, 3473, 3510, 3547, 3582, 3616, 3649,
    3681, 3712, 3741, 3770, 3797, 3823, 3848, 3872,
    3895, 3917, 3937, 3956, 3974, 3991, 4006, 4020,
    4033, 4045, 4056, 4065, 4073, 4080, 4086, 4090,
    4093, 4095, 4096,
};

/** Sine of a whole degree (any non-negative angle) from the quarter table. */
static inline int sinOfDegree(int degree)
{
    int index;
    if (degree % 180 < 90) {
        index = degree % 90;
    } else {
        index = 90 - degree % 90;
    }
    int sign;
    if (degree % 360 < 180) {
        sign = 1;
    } else {
        sign = -1;
    }
    return Fixed::SIN_COS_TABLE[index] * sign;
}

int Fixed::sqrt(int value)
{
    int root = (value + 0x1000) >> 1;
    for (int step = 0; step < 8; step++) {
        root = (div(value, root) + root) >> 1;
    }
    return root;
}

int Fixed::sin(int angle)
{
    // bring the angle into [0, 360) degrees plus one turn, so it is positive
    angle = angle % (360 << 12) + (360 << 12);
    int degree = angle >> 12;
    int value = sinOfDegree(degree);
    int nextDegree = (angle + 0x1000) >> 12;
    if (degree == nextDegree) {
        return value;
    }
    int nextValue = sinOfDegree(nextDegree);
    return value + (((nextValue - value) * (angle & 0xfff)) >> 12);
}
