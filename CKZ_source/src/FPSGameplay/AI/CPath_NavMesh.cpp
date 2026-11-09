// CPath_NavMesh.cpp - fully preserves the original capacity, sort count, floating-point unordered branches and path side effects.
#include "CPath_NavMesh.h"
#include "CGameAIMap_NavMesh.h"
#include "QuickSort.h"
#include "CAuxRender.h"

void CPath_NavMesh::SSmoothPoint::CalcAngleCoef()
{
    if (m_previous == NULL || m_current == NULL || m_next == NULL) {
        return;
    }
    vec3 previousDirection = m_previous->m_position - m_current->m_position;
    float previousScale = MathLib::InvSqrt(previousDirection.LengthSqr());
    vec3 nextDirection = m_next->m_position - m_current->m_position;
    float nextScale = MathLib::InvSqrt(nextDirection.LengthSqr());
    previousDirection = previousDirection * previousScale;
    nextDirection = nextDirection * nextScale;
    m_angleCoef = previousDirection.Dot(nextDirection);
}

CPath_NavMesh::CPath_NavMesh()
{
    m_pointCount = 0;
    for (int index = 0; index < 256; index++) {
        m_smoothPoints[index].m_previous = NULL;
        m_smoothPoints[index].m_current = NULL;
        m_smoothPoints[index].m_next = NULL;
        m_smoothPoints[index].m_angleCoef = 0.0f;
    }
    m_smoothCount = 0;
    m_state3c0c = 0;
    m_state3c10 = 0;
    m_flag3c14 = true;
    m_flag3c15 = true;
    m_body = NULL;
    m_cellCount = 0;
    m_lastCell = -1;
}

void CPath_NavMesh::Clear()
{
    m_pointCount = 0;
    m_cellCount = 0;
    m_lastCell = -1;
}

bool CPath_NavMesh::AddCell(int cellId)
{
    if (m_cellCount > 255) {
        return false;
    }
    m_cells[m_cellCount] = cellId;
    m_cellCount++;
    return true;
}

int CPath_NavMesh::GetCellID(int index) const
{
    if (index >= 0 && index < m_cellCount) {
        return m_cells[index];
    }
    return -1;
}

bool CPath_NavMesh::AddWayPoint(const vec3& position, int cellId)
{
    if (m_pointCount > 255) {
        return false;
    }
    m_points[m_pointCount].m_position = position;
    m_points[m_pointCount].m_gateEnds[0] = vec3();
    m_points[m_pointCount].m_gateEnds[1] = vec3();
    m_points[m_pointCount].m_cellId = cellId;
    m_points[m_pointCount].m_changed = false;
    m_pointCount++;
    return true;
}

bool CPath_NavMesh::AddGateWayPoint(const vec3& position, int cellId, const vec3& firstEnd, const vec3& secondEnd)
{
    if (m_pointCount > 255) {
        return false;
    }
    m_points[m_pointCount].m_position = position;
    m_points[m_pointCount].m_cellId = cellId;
    m_points[m_pointCount].m_gateEnds[0] = firstEnd;
    m_points[m_pointCount].m_gateEnds[1] = secondEnd;
    m_points[m_pointCount].m_changed = false;
    m_pointCount++;
    return true;
}

bool CPath_NavMesh::SmoothPoint(SSmoothPoint* point)
{
    if (point == NULL) {
        return false;
    }
    SWayPoint* current = point->m_current;
    vec3 oldPosition = current->m_position;
    const vec3& previous = point->m_previous->m_position;
    const vec3& next = point->m_next->m_position;
    const vec3& firstEnd = current->m_gateEnds[0];
    const vec3& secondEnd = current->m_gateEnds[1];

    // e6dca..e6e58: multiply-by-zero and addition grouping preserved; unordered values also keep the original branch, no swap to a standard line-intersection function.
    float deltaZ = previous.m_z - next.m_z;
    float deltaX = previous.m_x - next.m_x;
    float deltaY = previous.m_y - next.m_y;
    float zeroZ = deltaZ * 0.0f;
    float coefficientY = deltaX - zeroZ;
    float coefficientX = zeroZ - deltaY;
    float coefficientZ = deltaY * 0.0f - deltaX * 0.0f;
    float gateX = firstEnd.m_x - secondEnd.m_x;
    float gateY = firstEnd.m_y - secondEnd.m_y;
    float gateZ = firstEnd.m_z - secondEnd.m_z;
    float denominator = coefficientX * gateX + coefficientY * gateY;
    denominator = denominator + coefficientZ * gateZ;
    if (denominator == 0.0f) {
        return false;
    }
    float numerator = coefficientX * (previous.m_x - firstEnd.m_x)
                    + coefficientY * (previous.m_y - firstEnd.m_y);
    numerator = numerator + coefficientZ * (previous.m_z - firstEnd.m_z);
    float parameter = numerator / denominator;
    vec3 intersection(firstEnd.m_x + gateX * parameter,
                      firstEnd.m_y + gateY * parameter,
                      firstEnd.m_z + gateZ * parameter);
    float firstSide = (firstEnd.m_x - previous.m_x) * (intersection.m_y - previous.m_y)
                    - (firstEnd.m_y - previous.m_y) * (intersection.m_x - previous.m_x);
    float secondSide = (secondEnd.m_x - previous.m_x) * (intersection.m_y - previous.m_y)
                     - (secondEnd.m_y - previous.m_y) * (intersection.m_x - previous.m_x);
    if (firstSide * secondSide < 0.0f) {
        current->m_position = intersection;
    } else {
        vec3 firstDifference = firstEnd - intersection;
        vec3 secondDifference = secondEnd - intersection;
        if (firstDifference.LengthSqr() < secondDifference.LengthSqr()) {
            current->m_position = firstEnd;
        } else {
            current->m_position = secondEnd;
        }
    }
    point->m_current->m_changed = true;
    return !CGameAIMap_NavMesh::IsPositionTheSame(oldPosition, point->m_current->m_position);
}

void CPath_NavMesh::CreateSmoothInfo()
{
    if (m_pointCount > 255) {
        return;
    }
    m_smoothCount = 0;
    if (m_pointCount - 1 < 2) {
        return;
    }
    int index = 2;
    int last = m_pointCount - 1;
    SWayPoint* next = &m_points[index];
    do {
        m_smoothPoints[m_smoothCount].m_previous = next - 2;
        m_smoothPoints[m_smoothCount].m_current = next - 1;
        m_smoothPoints[m_smoothCount].m_next = next;
        m_smoothCount++;
        if (index >= last) {
            break;
        }
        index++;
        next++;
    } while (m_smoothCount < 256);
}

static int CompareTwoPathPoint(const void* first, const void* second);

void CPath_NavMesh::Smooth()
{
    if ((unsigned int)(m_smoothCount - 1) > 254) {
        return;
    }
    for (int index = 0; index < m_smoothCount; index++) {
        m_smoothPoints[index].CalcAngleCoef();
    }
    if (m_smoothCount == 1) {
        SmoothPoint(&m_smoothPoints[0]);
        return;
    }

    SWayPoint* lastChanged = NULL;
    for (int pass = 0; pass < m_smoothCount * 3; pass++) {
        QuickSort::Run(m_smoothPoints, m_smoothCount, sizeof(SSmoothPoint), CompareTwoPathPoint);
        for (int index = 0; index < m_smoothCount; index++) {
            if (m_smoothPoints[index].m_current == lastChanged) {
                index++; // Original e7060/e7062 double increment, not a plain continue.
            } else if (SmoothPoint(&m_smoothPoints[index])) {
                lastChanged = m_smoothPoints[index].m_current;
                for (int changedIndex = 0; changedIndex < m_smoothCount; changedIndex++) {
                    if (m_smoothPoints[changedIndex].m_current->m_changed) {
                        m_smoothPoints[changedIndex].CalcAngleCoef();
                    }
                }
                break;
            }
        }
    }
}

static int CompareTwoPathPoint(const void* first, const void* second)
{
    int result = 0;
    if (first != NULL && second != NULL) {
        const CPath_NavMesh::SSmoothPoint* firstPoint = (const CPath_NavMesh::SSmoothPoint*)first;
        const CPath_NavMesh::SSmoothPoint* secondPoint = (const CPath_NavMesh::SSmoothPoint*)second;
        if (firstPoint->m_angleCoef < secondPoint->m_angleCoef) {
            result = 1;
        }
    }
    return result;
}

CPath_NavMesh::SWayPoint* CPath_NavMesh::GetPathPoint(int index)
{
    return &m_points[index];
}

void CPath_NavMesh::Render(CAuxRender* render)
{
    if (render == NULL) {
        return;
    }
    for (int index = 0; index < m_pointCount - 1; index++) {
        render->DrawLine(m_points[index].m_position, m_points[index + 1].m_position, 0.3f, 0xff00ff00);
    }
}
