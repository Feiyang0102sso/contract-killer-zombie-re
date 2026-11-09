// CPath.cpp - all four methods of original u414; the random draw happens before the capacity check.
#include "CPath.h"
#include "WindowApp.h"

CPath::CPath()
{
    m_pointCount = 0;
    m_state1404 = 0;
    m_state1408 = 0;
    m_flag140c = true;
    m_flag140d = true;
}

void CPath::Clear()
{
    m_pointCount = 0;
}

bool CPath::AddPointInRadius(const vec3& position, int pointId, float edgeDistance, float radius)
{
    float radialRandom = WindowApp::m_instance->GetRandomValue().CFloat();
    float angleRandom = WindowApp::m_instance->GetRandomValue().CFloat();
    float sine;
    float cosine;
    MathLib::SinCos(angleRandom * 360.0f, sine, cosine);
    float radialDistance = radialRandom * radius;
    if (m_pointCount > 255) {
        return false;
    }
    m_points[m_pointCount].m_position = position + vec3(radialDistance * cosine, radialDistance * sine, 0.0f);
    m_points[m_pointCount].m_pointId = pointId;
    m_points[m_pointCount].m_edgeDistance = edgeDistance;
    m_pointCount++;
    return true;
}

CPath::SPathPoint* CPath::GetPathPoint(unsigned long index)
{
    return &m_points[index];
}
