// CMatrix4dh.cpp - self-orthonormalizing 4x4 matrix.
#include "CMatrix4dh.h"

#include <math.h>
#include "CMath.h"
#include "CVector3d.h"

namespace com { namespace glu { namespace platform { namespace math {

CMatrix4dh& CMatrix4dh::LoadIdentity()
{
    m[0] = 1.0f;
    m[1] = 0.0f;
    m[2] = 0.0f;
    m[3] = 0.0f;
    m[4] = 0.0f;
    m[5] = 1.0f;
    m[6] = 0.0f;
    m[7] = 0.0f;
    m[8] = 0.0f;
    m[9] = 0.0f;
    m[10] = 1.0f;
    m[11] = 0.0f;
    m[12] = 0.0f;
    m[13] = 0.0f;
    m[14] = 0.0f;
    m[15] = 1.0f;
    m_operationCount = 0;
    return *this;
}

CMatrix4dh& CMatrix4dh::Orthonormalize()
{
    CVector3d axisX(m[0], m[4], m[8]);
    CVector3d axisY(m[1], m[5], m[9]);

    axisX.Normalize();

    // remove the X component from Y, then normalize Y
    float dot = axisX.Dot(axisY);
    if (dot != 0.0f) {
        axisY.x = axisY.x - axisX.x * dot;
        axisY.y = axisY.y - axisX.y * dot;
        axisY.z = axisY.z - axisX.z * dot;
    }
    axisY.Normalize();

    CVector3d axisZ = axisX;
    axisZ.Cross(axisY);

    m[0] = axisX.x;
    m[1] = axisY.x;
    m[2] = axisZ.x;
    m[4] = axisX.y;
    m[5] = axisY.y;
    m[6] = axisZ.y;
    m[8] = axisX.z;
    m[9] = axisY.z;
    m[10] = axisZ.z;
    m_operationCount = 0;
    return *this;
}

CMatrix4dh& CMatrix4dh::RotateZ(float degrees)
{
    float radians = CMath::PI * degrees / 180.0f;
    float sine = sinf(radians);
    float cosine = cosf(radians);

    // row 0 is overwritten first, keep its old values for row 1
    float m0 = m[0];
    float m1 = m[1];
    float m2 = m[2];

    m[0] = m0 * cosine + m[4] * sine;
    m[1] = m1 * cosine + m[5] * sine;
    m[2] = m2 * cosine + m[6] * sine;
    m[4] = m[4] * cosine - m0 * sine;
    m[5] = m[5] * cosine - m1 * sine;
    m[6] = m[6] * cosine - m2 * sine;

    if (m_operationCount != 0xFFFF) {
        m_operationCount++;
    }
    if (m_autoOrthonormalize && m_operationCount >= m_orthonormalizeInterval) {
        Orthonormalize();
    }
    return *this;
}

} } } }
