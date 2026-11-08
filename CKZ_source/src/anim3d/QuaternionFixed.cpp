// QuaternionFixed.cpp - fixed point quaternion operations (created by b047x).
#include "QuaternionFixed.h"
#include "Fixed.h"

void QuaternionFixed::set(QuaternionFixed* quaternion, int w, int x, int y, int z)
{
    quaternion->m_w = w;
    quaternion->m_x = x;
    quaternion->m_y = y;
    quaternion->m_z = z;
}

void QuaternionFixed::inverse(QuaternionFixed* quaternion)
{
    normalize(quaternion);
    set(quaternion, -quaternion->m_w, -quaternion->m_x, -quaternion->m_y, -quaternion->m_z);
}

void QuaternionFixed::normalize(QuaternionFixed* quaternion)
{
    int length = Fixed::sqrt(Fixed::mul(quaternion->m_x, quaternion->m_x) + Fixed::mul(quaternion->m_y, quaternion->m_y)
                             + Fixed::mul(quaternion->m_z, quaternion->m_z) + Fixed::mul(quaternion->m_w, quaternion->m_w));
    if (length != 0) {
        int scale = Fixed::div(0x1000, length);
        quaternion->m_w = Fixed::mul(quaternion->m_w, scale);
        quaternion->m_x = Fixed::mul(quaternion->m_x, scale);
        quaternion->m_y = Fixed::mul(quaternion->m_y, scale);
        quaternion->m_z = Fixed::mul(quaternion->m_z, scale);
    }
}

int QuaternionFixed::dot(QuaternionFixed* left, QuaternionFixed* right)
{
    return Fixed::mul(left->m_x, right->m_x) + Fixed::mul(left->m_y, right->m_y)
           + Fixed::mul(left->m_z, right->m_z) + Fixed::mul(left->m_w, right->m_w);
}

void QuaternionFixed::interpolate(QuaternionFixed* from, QuaternionFixed* to, int factor)
{
    from->m_w = from->m_w + Fixed::mul(to->m_w - from->m_w, factor);
    from->m_x = from->m_x + Fixed::mul(to->m_x - from->m_x, factor);
    from->m_y = from->m_y + Fixed::mul(to->m_y - from->m_y, factor);
    from->m_z = from->m_z + Fixed::mul(to->m_z - from->m_z, factor);
    normalize(from);
}

void QuaternionFixed::toMatrix(QuaternionFixed* quaternion, float* matrix)
{
    int x2 = quaternion->m_x * 2;
    int y2 = quaternion->m_y * 2;
    int z2 = quaternion->m_z * 2;

    int xx = Fixed::mul(quaternion->m_x, x2);
    int xy = Fixed::mul(quaternion->m_x, y2);
    int xz = Fixed::mul(quaternion->m_x, z2);
    int yy = Fixed::mul(quaternion->m_y, y2);
    int yz = Fixed::mul(quaternion->m_y, z2);
    int zz = Fixed::mul(quaternion->m_z, z2);
    int wx = Fixed::mul(quaternion->m_w, x2);
    int wy = Fixed::mul(quaternion->m_w, y2);
    int wz = Fixed::mul(quaternion->m_w, z2);

    matrix[0] = Fixed::toFloat(0x1000 - yy - zz);
    matrix[1] = Fixed::toFloat(xy - wz);
    matrix[2] = Fixed::toFloat(xz + wy);
    matrix[3] = 0.0f;
    matrix[4] = Fixed::toFloat(xy + wz);
    matrix[5] = Fixed::toFloat(0x1000 - xx - zz);
    matrix[6] = Fixed::toFloat(yz - wx);
    matrix[7] = 0.0f;
    matrix[8] = Fixed::toFloat(xz - wy);
    matrix[9] = Fixed::toFloat(yz + wx);
    matrix[10] = Fixed::toFloat(0x1000 - xx - yy);
    matrix[11] = 0.0f;
    matrix[12] = 0.0f;
    matrix[13] = 0.0f;
    matrix[14] = 0.0f;
    matrix[15] = 1.0f;
}
