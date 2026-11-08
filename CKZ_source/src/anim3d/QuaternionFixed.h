// QuaternionFixed.h - rotation quaternion in 20.12 fixed point (created by b047x).
// All operations are static and take the quaternion as a pointer (C style API).
#ifndef QUATERNIONFIXED_H
#define QUATERNIONFIXED_H

class QuaternionFixed {
public:
    /** The identity rotation. */
    QuaternionFixed()
    {
        set(this, 0x1000, 0, 0, 0);
    }

    static void set(QuaternionFixed* quaternion, int w, int x, int y, int z);
    /** Normalize, then negate all four components. */
    static void inverse(QuaternionFixed* quaternion);
    static void normalize(QuaternionFixed* quaternion);
    static int dot(QuaternionFixed* left, QuaternionFixed* right);
    /** Linear interpolation of `from` towards `to` by `factor` (20.12), then normalize. */
    static void interpolate(QuaternionFixed* from, QuaternionFixed* to, int factor);
    /** Row major 4x4 rotation matrix. */
    static void toMatrix(QuaternionFixed* quaternion, float* matrix);

    int m_w;    // 0x00
    int m_x;    // 0x04
    int m_y;    // 0x08
    int m_z;    // 0x0c
};

#endif
