// CVector3d.h - 3-component float vector.
#ifndef CVECTOR3D_H
#define CVECTOR3D_H

#include <math.h>

namespace com { namespace glu { namespace platform { namespace math {

class CVector3d {
public:
    CVector3d()
    {
    }

    // Empty user destructor: the original has EH cleanups for CVector3d members in ObjC++ code
    ~CVector3d() {}  // added by b003

    CVector3d(float initX, float initY, float initZ)
    {
        x = initX;
        y = initY;
        z = initZ;
    }

    float Dot(const CVector3d& other) const
    {
        return x * other.x + y * other.y + z * other.z;
    }

    float Length() const
    {
        return sqrtf(x * x + y * y + z * z);
    }

    /** Scale to unit length; a zero vector is left unchanged. */
    void Normalize()
    {
        float length = Length();
        if (length != 0.0f) {
            x = x / length;
            y = y / length;
            z = z / length;
        }
    }

    /** this = this x other. */
    CVector3d& Cross(const CVector3d& other);

    float x;    // 0x00
    float y;    // 0x04
    float z;    // 0x08
};

} } } }

#endif
