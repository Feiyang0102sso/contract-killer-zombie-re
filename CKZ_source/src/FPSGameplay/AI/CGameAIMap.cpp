// CGameAIMap.cpp - original u412 full node-graph loading, visibility grouping, A* and debug drawing.
#include "CGameAIMap.h"
#include "CPath.h"
#include "App.h"
#include "DGHelper.h"
#include "CSwerveGame.h"
#include "dgCamera.h"
#include "CBH_Player.h"
#include "CAuxRender.h"
#include "CAuxRenderText.h"
#include "ICStdUtil.h"
#include "CRandGen.h"

void IGameAIMap::LocationFromName(Location& location, const XString& name)
{
    LocationFromId(location, GetPointIDByName(name));
}

bool CGameAIMap::SyncLocation(Location& location, const vec3& position) const
{
    if (location.m_map != NULL) {
        vec3 difference = position - location.m_position;
        // Original e1b20 BLT includes unordered; must not be written as a plain less-than that changes the NaN path.
        if (!(difference.LengthSqr() >= location.m_syncDistance)) {
            return true;
        }
    }
    float nearestDistance;
    int nearestId;
    if (FindNearestPoint(&nearestId, &nearestDistance, position)) {
        location.m_map = const_cast<CGameAIMap*>(this);
        location.m_pointId = nearestId;
        location.m_position = position;
        location.m_syncDistance = 0.0f;
    } else {
        location.m_map = NULL;
    }
    // Original e1b4a: returns true even when the nearest point fails; only m_map indicates no location.
    return true;
}

bool CGameAIMap::FindNearestPoint(int* pointId, float* distance, const vec3& position) const
{
    bool found = false;
    int pointCount = m_points.size();
    float bestDistanceSquared = 2147483648.0f; // Original e1c1c: 0x4f000000
    for (int index = 0; index < pointCount; index++) {
        if (m_points[index].m_usable) {
            vec3 difference = m_points[index].m_position - position;
            float distanceSquared = difference.LengthSqr();
            if (distanceSquared < bestDistanceSquared) {
                *pointId = m_points[index].m_id;
                *distance = MathLib::InvSqrt(distanceSquared) * distanceSquared;
                found = true;
                bestDistanceSquared = distanceSquared;
                pointCount = m_points.size();
            }
        }
    }
    return found;
}

bool CGameAIMap::FindPath(CSimplePath& result, const Location& start, const Location& end,
                          float radius, unsigned int flags)
{
    CPath path;
    if (!FindPathAStarImpl(&path, &m_points[start.m_pointId], &m_points[end.m_pointId])) {
        return false;
    }
    int pointCount = path.m_pointCount;
    if (pointCount > 0) {
        result.m_pointCount = pointCount;
        for (int index = 1; index < pointCount; index++) {
            CPath::SPathPoint* point = path.GetPathPoint(index);
            if (index == pointCount - 1) {
                result.m_points[index].m_pointId = start.m_pointId;
                result.m_points[index].m_position = start.m_position;
            } else {
                result.m_points[index].m_pointId = point->m_pointId;
                result.m_points[index].m_position = point->m_position;
            }
            result.m_points[index].m_distance = 0.1f;
        }
    } else {
        result.m_pointCount = 1;
    }
    result.m_points[0].m_pointId = end.m_pointId;
    result.m_points[0].m_position = end.m_position;
    result.m_points[0].m_distance = 0.1f;
    return true;
}

bool CGameAIMap::Pick(Location& location, int x, int y, swerve::Camera<CSwerveAssertingErrorHandler>& camera) const
{
    // Original e1d00/4B is only movs r0,#0; bx lr.
    return false;
}

void CGameAIMap::LocationFromId(Location& location, int pointId) const
{
    location.m_map = const_cast<CGameAIMap*>(this);
    location.m_position = GetMapPointPostition(pointId);
    location.m_pointId = pointId;
    location.m_syncDistance = 0.0f;
}

void CGameAIMap::FindObstacles(const Location& location, ObstaclesContainer& obstacles)
{
    // Original e1d34/4B is only bx lr and nop; this node graph has no static boundary obstacle collection.
}

void CGameAIMap::DefineGameCharacteristic(SGameAIPoint* point, swerve::Node<CSwerveAssertingErrorHandler>& node)
{
    char parameterName[1000];
    int spawnType;
    point->m_pointType = 0;
    com::glu::platform::core::ICStdUtil::SPrintF(parameterName, "SpawnType");
    ((App*)WindowApp::m_instance)->m_helper->getUserParameter(node, parameterName, &spawnType);
    // Clamp the stored member; the original leaves the parsed stack value unchanged.
    if (spawnType < 0) {
        point->m_spawnType = 0;
    } else {
        point->m_spawnType = spawnType;
    }

    com::glu::platform::core::ICStdUtil::SPrintF(parameterName, "PointName");
    char* pointName = NULL;
    ((App*)WindowApp::m_instance)->m_helper->getUserParameter(node, parameterName, &pointName);
    if (pointName != NULL) {
        point->m_name = XString(pointName);
        if (pointName != NULL) {
            free(pointName);
            pointName = NULL;
        }
    }

    com::glu::platform::core::ICStdUtil::SPrintF(parameterName, "PointRadius");
    float pointRadius;
    ((App*)WindowApp::m_instance)->m_helper->getUserParameter(node, parameterName, &pointRadius);
    point->m_radius = pointRadius;
    point->m_state60 = 0;
    com::glu::platform::core::ICStdUtil::SPrintF(parameterName, "SceneCameraCount");
    int cameraCount;
    ((App*)WindowApp::m_instance)->m_helper->getUserParameter(node, parameterName, &cameraCount);
    if (cameraCount > 0) {
        point->m_cameraAttitudes.ensureCapacity(cameraCount);
        for (int index = 0; index < cameraCount; index++) {
            SCameraAttitude attitude;
            int visibility;
            int safety;
            int startLevel;
            com::glu::platform::core::ICStdUtil::SPrintF(parameterName, "cameraVisibility_%d", index);
            ((App*)WindowApp::m_instance)->m_helper->getUserParameter(node, parameterName, &visibility);
            attitude.m_visible = visibility == 1;
            com::glu::platform::core::ICStdUtil::SPrintF(parameterName, "cameraSafity_%d", index);
            ((App*)WindowApp::m_instance)->m_helper->getUserParameter(node, parameterName, &safety);
            attitude.m_safe = safety == 1;
            com::glu::platform::core::ICStdUtil::SPrintF(parameterName, "cameraSafityType_%d", index);
            ((App*)WindowApp::m_instance)->m_helper->getUserParameter(node, parameterName, &attitude.m_safetyType);
            com::glu::platform::core::ICStdUtil::SPrintF(parameterName, "cameraStartLevel_%d", index);
            ((App*)WindowApp::m_instance)->m_helper->getUserParameter(node, parameterName, &startLevel);
            if (startLevel <= 0) {
                startLevel = 1;
            }
            attitude.m_startLevel = startLevel;
            point->m_cameraAttitudes.addElement(attitude);
        }
    }
}

void CGameAIMap::FindSpecialPoints()
{
    int pointCount = m_points.size();
    int regularCount = 0;
    int typeOneCount = 0;
    int typeTwoCount = 0;
    int cameraCount = 0;
    for (int index = 0; index < pointCount; index++) {
        const SGameAIPoint& point = m_points[index];
        if (point.m_usable) {
            switch (point.m_pointType) {
            case 0:
                regularCount++;
                break;
            case 1:
                typeOneCount++;
                break;
            case 2:
                typeTwoCount++;
                break;
            }
            if (point.m_cameraAttitudes.size() > cameraCount) {
                cameraCount = point.m_cameraAttitudes.size();
            }
        }
    }
    m_cameraPoints.ensureCapacity(cameraCount);
    m_regularPoints.ensureCapacity(regularCount);
    m_typeOnePoints.ensureCapacity(typeOneCount);
    m_typeTwoPoints.ensureCapacity(typeTwoCount);
    for (int index = 0; index < pointCount; index++) {
        if (m_points[index].m_usable) {
            switch (m_points[index].m_pointType) {
            case 0:
                m_regularPoints.addElement(index);
                break;
            case 1:
                m_typeOnePoints.addElement(index);
                break;
            case 2:
                m_typeTwoPoints.addElement(index);
                break;
            }
        }
    }
    int level = CBH_Player::GetInstance()->m_id;
    for (int index = 0; index < cameraCount; index++) {
        m_cameraPoints.addElement(Vector<int>());
    }
    for (int pointIndex = 0; pointIndex < pointCount; pointIndex++) {
        SGameAIPoint& point = m_points[pointIndex];
        if (point.m_pointType == 0) {
            for (int cameraIndex = 0; cameraIndex < point.m_cameraAttitudes.size(); cameraIndex++) {
                if (point.m_cameraAttitudes[cameraIndex].m_startLevel > level) {
                    point.m_cameraAttitudes[cameraIndex].m_visible = false;
                }
                if (point.m_cameraAttitudes[cameraIndex].m_visible) {
                    m_cameraPoints[cameraIndex].addElement(pointIndex);
                }
            }
        }
    }
}

bool CGameAIMap::Init(swerve::World<CSwerveAssertingErrorHandler>& world)
{
    swerve::Group<CSwerveAssertingErrorHandler> root(world.find(DGHelper::getSwerveID("AI_RootTransform")));
    if (root.m_interface == NULL) {
        return false;
    }
    int pointCount = root.getChildCount();
    m_points.ensureCapacity(pointCount);
    for (int index = 0; index < pointCount; index++) {
        SGameAIPoint point;
        point.m_id = index;
        m_points.addElement(point);
    }
    for (int index = 0; index < pointCount; index++) {
        swerve::Node<CSwerveAssertingErrorHandler> node = root.getChild(index);
        int pointId;
        ((App*)WindowApp::m_instance)->m_helper->getUserParameter(node, "ID", &pointId);
        SGameAIPoint* point = &m_points[pointId];
        // Original e2260/e2284 first has the external call write a stack temporary, then copies it back to the cached node address.
        float translation[3];
        node.getTranslation(translation);
        point->m_position.m_x = translation[0];
        point->m_position.m_y = translation[1];
        point->m_position.m_z = translation[2];
        float orientation[4];
        node.getOrientation(orientation);
        *(Quaternion*)point->m_orientation = *(const Quaternion*)orientation;
        DefineGameCharacteristic(point, node);
    }
    for (int index = 0; index < pointCount; index++) {
        swerve::Node<CSwerveAssertingErrorHandler> node = root.getChild(index);
        int linkCount = 0;
        int pointId;
        char parameterName[1000];
        ((App*)WindowApp::m_instance)->m_helper->getUserParameter(node, "LinksCount", &linkCount);
        ((App*)WindowApp::m_instance)->m_helper->getUserParameter(node, "ID", &pointId);
        SGameAIPoint* point = &m_points[pointId];
        if (point->m_pointType != 2) {
            point->m_edges.ensureCapacity(linkCount);
            for (int linkIndex = 0; linkIndex < linkCount; linkIndex++) {
                int linkedId;
                com::glu::platform::core::ICStdUtil::SPrintF(parameterName, "LinkID_%d", linkIndex);
                ((App*)WindowApp::m_instance)->m_helper->getUserParameter(node, parameterName, &linkedId);
                if (linkedId >= 0) {
                    SGameAIPoint* target = &m_points[linkedId];
                    if (target->m_id != point->m_id) {
                        vec3 difference = point->m_position - target->m_position;
                        float distanceSquared = difference.LengthSqr();
                        SGameAIEdge edge;
                        edge.m_target = target;
                        edge.m_distance = MathLib::InvSqrt(distanceSquared) * distanceSquared;
                        point->m_edges.addElement(edge);
                    }
                }
            }
            point->m_usable = point->m_edges.size() != 0;
        }
    }
    FindSpecialPoints();
    m_openList.reserve(pointCount);
    return true;
}

bool CGameAIMap::FindPathAStarImpl(CPath* path, SGameAIPoint* start, SGameAIPoint* end)
{
    if (start == NULL || end == NULL) {
        return false;
    }
    path->Clear();
    if (start == end) {
        return true;
    }
    InitializeAStar();
    bool found = false;
    OpenListPushBack(start, end, NULL, 0.0f);
    if (m_openList.m_size == 0) {
        return false;
    }
    while (m_openList.m_size != 0 && !found) {
        SGameAIPoint* current = OpenListPop();
        int edgeCount = current->m_edges.size();
        for (int index = 0; index < edgeCount; index++) {
            SGameAIPoint* target = current->m_edges[index].m_target;
            float edgeDistance = current->m_edges[index].m_distance;
            if (target->m_searchState == 0) {
                OpenListPushBack(target, end, current, edgeDistance);
                if (target == end) {
                    found = true;
                }
            } else if (target->m_searchState == 1) {
                if (current->m_pathDistance + edgeDistance < target->m_pathDistance) {
                    // Original e251c..252c update formula; do not arbitrarily change it to current.g + edgeDistance as in standard A*.
                    float previousDistance = target->m_pathDistance - target->m_parentEdgeDistance;
                    target->m_parentEdgeDistance = edgeDistance;
                    target->m_pathDistance = previousDistance + edgeDistance;
                    target->m_parent = current;
                }
            }
        }
    }
    if (found) {
        ConstructPath(path, end);
        return true;
    }
    return false;
}

int CGameAIMap::GetPointIDImpl(const Vector<int>& points) const
{
    int count = points.size();
    if (m_points.size() < 1 || count < 1) {
        return -1;
    }
    int index = com::glu::platform::core::CRandGen::GetInstance()->GetRandRange(0, count - 1);
    return points[index];
}

bool CGameAIMap::FindAnotherPoint(int* pointId, int previousPointId) const
{
    int pointCount = m_points.size();
    if (pointCount < 2) {
        return false;
    }
    bool found = false;
    int attempts = 0;
    do {
        int candidate = GetPointIDImpl(m_regularPoints);
        if (candidate != previousPointId) {
            *pointId = candidate;
            found = true;
        }
        attempts++;
    } while (attempts < pointCount || !found);
    return true;
}

const vec3& CGameAIMap::GetMapPointPostition(int pointId) const
{
    return m_points[pointId].m_position;
}

const float* CGameAIMap::GetMapPointOrientation(int pointId) const
{
    return m_points[pointId].m_orientation;
}

int CGameAIMap::GetHideBehaviour(int pointId, int cameraId) const
{
    return m_points[pointId].m_cameraAttitudes[cameraId].m_safetyType;
}

void CGameAIMap::ConstructPath(CPath* path, SGameAIPoint* point)
{
    float edgeDistance = 0.0f;
    while (point != NULL) {
        SGameAIPoint* parent = point->m_parent;
        path->AddPointInRadius(point->m_position, point->m_id, edgeDistance, point->m_radius);
        if (parent != NULL) {
            for (int index = 0; index < parent->m_edges.size(); index++) {
                if (point == parent->m_edges[index].m_target) {
                    edgeDistance = parent->m_edges[index].m_distance;
                    break;
                }
            }
        }
        point = parent;
    }
}

void CGameAIMap::InitializeAStar()
{
    int pointCount = m_points.size();
    for (int index = 0; index < pointCount; index++) {
        SGameAIPoint& point = m_points[index];
        point.m_pathDistance = 0.0f;
        point.m_heuristicDistance = 0.0f;
        point.m_parentEdgeDistance = 0.0f;
        point.m_parent = NULL;
        point.m_searchState = 0;
    }
    m_openList.m_size = 0;
}

void CGameAIMap::OpenListPushBack(SGameAIPoint* point, SGameAIPoint* end, SGameAIPoint* parent, float edgeDistance)
{
    vec3 difference = end->m_position - point->m_position;
    float distanceSquared = difference.LengthSqr();
    point->m_parentEdgeDistance = edgeDistance;
    point->m_parent = parent;
    point->m_searchState = 1;
    point->m_heuristicDistance = MathLib::InvSqrt(distanceSquared) * distanceSquared;
    point->m_pathDistance = point->m_parentEdgeDistance;
    if (parent != NULL) {
        point->m_pathDistance = point->m_pathDistance + parent->m_pathDistance;
    }
    if (m_openList.m_size < m_openList.m_capacity) {
        m_openList.m_data[m_openList.m_size] = point;
        m_openList.m_size++;
    }
}

CGameAIMap::SGameAIPoint* CGameAIMap::OpenListPop()
{
    SGameAIPoint* bestPoint = NULL;
    int count = m_openList.m_size;
    if (count > 0) {
        int bestIndex = 0;
        bestPoint = m_openList.m_data[0];
        if (count >= 2) {
            float bestCost = bestPoint->m_pathDistance + bestPoint->m_heuristicDistance;
            for (int index = 1; index < count; index++) {
                SGameAIPoint* candidate = m_openList.m_data[index];
                float cost = candidate->m_pathDistance + candidate->m_heuristicDistance;
                if (cost < bestCost) {
                    bestCost = cost;
                    bestPoint = candidate;
                    bestIndex = index;
                }
            }
        }
        m_openList.remove_at(bestIndex);
        bestPoint->m_searchState = 2;
    }
    return bestPoint;
}

void CGameAIMap::RenderMap(CAuxRender* render, int drawRadii)
{
    if (render == NULL) {
        return;
    }
    vec3 boxSize(0.5f, 0.5f, 0.5f);
    int pointCount = m_points.size();
    for (int pointIndex = 0; pointIndex < pointCount; pointIndex++) {
        SGameAIPoint* point = &m_points[pointIndex];
        vec3 position = point->m_position;
        if (point->m_pointType == 0) {
            if (point->m_cameraAttitudes.size() > 0 && point->m_cameraAttitudes[0].m_safe) {
                render->DrawBox(position, boxSize, 0xffff0000);
            } else if (point->m_cameraAttitudes.size() > 0 && point->m_cameraAttitudes[0].m_visible) {
                render->DrawBox(position, boxSize, 0xff7f0000);
            } else {
                render->DrawBox(position, boxSize, 0xffdcdcdc);
            }
        } else if (point->m_pointType == 1) {
            render->DrawBox(position, boxSize, 0xff0000ff);
        } else if (point->m_pointType == 2) {
            render->DrawBox(position, boxSize, 0xffff6400);
        }
        if (drawRadii == 1) {
            render->DrawSphere(position, point->m_radius, 0xff7f0000);
        }
        RenderPointInformation(*point);
        int edgeCount = point->m_edges.size();
        for (int edgeIndex = 0; edgeIndex < edgeCount; edgeIndex++) {
            SGameAIPoint* target = point->m_edges[edgeIndex].m_target;
            if (target == NULL) {
                continue;
            }
            render->DrawLine(position, target->m_position, 0.1f, 0xffffff00);
            if (drawRadii == 1) {
                vec3 difference = target->m_position - position;
                float distanceSquared = difference.LengthSqr();
                // Original loads both radii before the InvSqrt call.
                float targetRadius = target->m_radius;
                float pointRadius = point->m_radius;
                float distance = MathLib::InvSqrt(distanceSquared) * distanceSquared;
                float angle = MathLib::ArcSin(MathLib::Fabs(targetRadius - pointRadius) / distance);
                if (target->m_radius < point->m_radius) {
                    angle = 180.0f - angle;
                }
                float secondAngle = angle + -90.0f;
                float cosine = MathLib::Cos(angle);
                float sine = MathLib::Sin(angle);
                vec3 firstStart(position.m_x + cosine * point->m_radius,
                                position.m_y + sine * point->m_radius,
                                position.m_z + point->m_radius * 0.1f);
                cosine = MathLib::Cos(angle);
                sine = MathLib::Sin(angle);
                vec3 firstEnd(target->m_position.m_x + cosine * target->m_radius,
                              target->m_position.m_y + sine * target->m_radius,
                              target->m_position.m_z + target->m_radius * 0.1f);
                cosine = MathLib::Cos(secondAngle);
                sine = MathLib::Sin(secondAngle);
                vec3 secondStart(position.m_x + cosine * point->m_radius,
                                 position.m_y + sine * point->m_radius,
                                 position.m_z + point->m_radius * 0.1f);
                cosine = MathLib::Cos(secondAngle);
                sine = MathLib::Sin(secondAngle);
                vec3 secondEnd(target->m_position.m_x + cosine * target->m_radius,
                               target->m_position.m_y + sine * target->m_radius,
                               target->m_position.m_z + target->m_radius * 0.1f);
                render->DrawLine(firstStart, firstEnd, 0.1f, 0xffdcdcdc);
                render->DrawLine(secondStart, secondEnd, 0.1f, 0xffdcdcdc);
            }
        }
    }
}

void CGameAIMap::RenderPointInformation(const SGameAIPoint& point)
{
    DGCamera* camera = ((App*)WindowApp::m_instance)->m_helper->m_swerveGame->m_camera;
    if (camera == NULL) {
        return;
    }
    int x = 0;
    int y = 0;
    if (camera->GetScreenPosition(&x, &y, point.m_position)) {
        XString text = XString::Format(L"P%d", point.m_id);
        if (point.m_name.ByteSize() != 0) {
            text.Append(XString::Format(L" (%s)", point.m_name.Replace(L"_", L" ", true).m_data));
        }
        CAuxRenderText::GetInstance()->AddTextInfoXY(x, y, text.m_data);
    }
}

int CGameAIMap::GetPointIDByName(const XString& name) const
{
    for (int index = 0; index < m_points.size(); index++) {
        if (m_points[index].m_name == name) {
            return m_points[index].m_id;
        }
    }
    // Original e2cee..e2e0e still builds the full diagnostic text, even though the release build never actually outputs it.
    XString message = XString::Format(L"Can't find map point with name \"%s\"", name.m_data);
    message = message + L"\n";
    message = message + L"Valid points are: \n";
    for (int index = 0; index < m_points.size(); index++) {
        message = message + m_points[index].m_name;
        message = message + L" ";
    }
    return 0;
}

int CGameAIMap::GetPointSpawnType(int pointId) const
{
    for (int index = 0; index < m_points.size(); index++) {
        if (m_points[index].m_id == pointId) {
            return m_points[index].m_spawnType;
        }
    }
    XString message = XString::Format(L"Can't find map point with id \"%d\"", pointId);
    return 0;
}
