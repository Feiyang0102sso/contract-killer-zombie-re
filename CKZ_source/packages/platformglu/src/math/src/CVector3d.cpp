// CVector3d.cpp - out-of-line CVector3d operations.
#include "CVector3d.h"

namespace com { namespace glu { namespace platform { namespace math {

CVector3d& CVector3d::Cross(const CVector3d& other)
{
    // computed into a temporary first: `other` may be this vector
    CVector3d result(y * other.z - z * other.y,
                     z * other.x - x * other.z,
                     x * other.y - y * other.x);
    *this = result;
    return *this;
}

} } } }
