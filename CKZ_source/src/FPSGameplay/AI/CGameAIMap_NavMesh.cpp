// CGameAIMap_NavMesh.cpp - original u413 navigation mesh, gate edges, paths and debug drawing.
#include "CGameAIMap_NavMesh.h"
#include "CPath_NavMesh.h"
#include "CUnitBody.h"
#include "App.h"
#include "DGHelper.h"
#include "CSwerveGame.h"
#include "CSwerve.h"
#include "dgCamera.h"
#include "CBH_Player.h"
#include "CAuxRender.h"
#include "CAuxRenderText.h"
#include "ICStdUtil.h"
#include "CRandGen.h"
#include "Point.h"

bool BelongsToCell(const CGameAIMap_NavMesh::SCellAI& cell, const vec3& position, float length, float& distance);
bool ProjectToCell(const CGameAIMap_NavMesh::SCellAI& cell, vec3& position, float length);

const float CGameAIMap_NavMesh::SAME_POINT_THRESHOLD = 0.001f; // Original 45d0ec, only 4B

int CGameAIMap_NavMesh::FindNearestCell(const vec3& position) const
{
    int nearest = -1;
    int projected = -1;
    float bestSquared = 1.0e18f;
    float bestProjectionDistance = 1.0e18f;
    for (int index = 0; index < m_cells.size(); index++) {
        const SCellAI& cell = m_cells[index];
        vec3 difference = cell.m_center - position;
        float squared = difference.LengthSqr();
        float distance = 100.0f;
        if (squared < bestSquared) {
            bestSquared = squared;
            nearest = index;
        }
        if (BelongsToCell(cell, position, 100.0f, distance) && distance < bestProjectionDistance) {
            bestProjectionDistance = distance;
            projected = index;
        }
    }
    if (projected != -1) {
        nearest = projected;
    }
    return nearest;
}

void CGameAIMap_NavMesh::DefineGatesForPath(CPath_NavMesh& path)
{
    int count = path.m_cellCount;
    for (int index = 0; index < count - 1; index++) {
        int firstId = path.GetCellID(index);
        int secondId = path.GetCellID(index + 1);
        if (firstId < 0 || firstId >= m_cells.size() || secondId < 0 || secondId >= m_cells.size()) {
            continue;
        }
        SCellAI* firstCell = &m_cells[firstId];
        SCellAI* secondCell = &m_cells[secondId];
        if (firstCell == NULL || secondCell == NULL) {
            continue;
        }
        float radius = path.m_body->GetRadius();
        for (int edge = 0; edge < 3; edge++) {
            if (firstCell->m_neighbours[edge] != secondCell) {
                continue;
            }
            SGateAI* gate = firstCell->m_gates[edge];
            vec3 first = gate->m_vertices[0]->m_position;
            vec3 second = gate->m_vertices[1]->m_position;
            vec3 difference = first - second;
            float squared = difference.LengthSqr();
            float length = MathLib::InvSqrt(squared) * squared;
            // Original e4f3c BPL also enters the shrink branch for unordered.
            if (length < radius + radius) {
                vec3 middle = (first + second) * .5f;
                path.AddGateWayPoint(middle, firstCell->m_id, middle, middle);
            } else {
                vec3 directions[2];
                directions[0] = second - first;
                directions[0] = directions[0] * MathLib::InvSqrt(directions[0].LengthSqr());
                directions[1] = difference * MathLib::InvSqrt(difference.LengthSqr());
                vec3 ends[2];
                ends[0] = first;
                ends[1] = second;
                for (int endpoint = 0; endpoint < 2; endpoint++) {
                    if (firstCell->m_gates[edge]->m_vertices[endpoint]->m_border) {
                        ends[endpoint] = ends[endpoint] + directions[endpoint] * radius;
                    }
                }
                vec3 middle = (ends[0] + ends[1]) * .5f;
                path.AddGateWayPoint(middle, firstCell->m_id, ends[0], ends[1]);
            }
            break;
        }
    }
}

void CGameAIMap_NavMesh::SmoothPath(CPath_NavMesh& path)
{
    path.CreateSmoothInfo();
    if (path.m_smoothCount > 0) {
        path.Smooth();
    }
}

void CGameAIMap_NavMesh::ConstructPrecisePath(CPath_NavMesh& path, SCellAI* start)
{
    if (start != NULL) {
        path.AddWayPoint(start->m_center, -1);
    }
    DefineGatesForPath(path);
    SmoothPath(path);
}

bool CGameAIMap_NavMesh::FindPathAStarImpl(CPath_NavMesh& path, SCellAI* start, SCellAI* end,
                                         float radius, unsigned int flags)
{
    if (start == NULL || end == NULL) {
        return false;
    }
    if (end == start) {
        // Original e5294 returns true when start is in the same cell, without clearing the existing path.
        return true;
    }
    path.Clear();
    AStarInit();
    bool found = false;
    OpenCellListPushBack(start, end, NULL, 0.0f);
    while (m_openList.m_size != 0 && !found) {
        SCellAI* cell = OpenCellListPop();
        if (cell == end) {
            found = true;
            break;
        }
        for (int edge = 0; edge < 3; edge++) {
            SCellAI* neighbour = cell->m_neighbours[edge];
            if (neighbour == NULL) {
                continue;
            }
            SGateAI* gate = cell->m_gates[edge];
            if (gate == NULL) {
                return false;
            }
            flags += gate->m_randomValue;
            float factor = (float)(flags % 1000) * .001f;
            factor = factor * radius + 1.0f;
            float cost = gate->m_distance * factor;
            if (neighbour->m_searchState == 0) {
                OpenCellListPushBack(neighbour, end, cell, cost);
                if (neighbour == end) {
                    found = true;
                }
            } else if (neighbour->m_searchState == 1) {
                if (cell->m_pathDistance + cost < neighbour->m_pathDistance) {
                    float previousCost = neighbour->m_parentEdgeDistance;
                    neighbour->m_parentEdgeDistance = cost;
                    // Original e53a2 keeps old g minus old edge plus new edge; not the standard A* current.g+cost.
                    neighbour->m_pathDistance = (neighbour->m_pathDistance - previousCost) + cost;
                    neighbour->m_parent = cell;
                }
            }
        }
    }
    if (found) {
        while (end != NULL) {
            int cellId = end->m_id;
            end = end->m_parent;
            path.AddCell(cellId);
        }
    }
    return found;
}

bool CGameAIMap_NavMesh::SyncLocation(Location& location, const vec3& position) const
{
    location.m_syncDistance = 0.0f;
    location.m_position = position;
    if (location.m_map == this && location.m_pointId >= 0 && location.m_pointId < m_cells.size()) {
        const SCellAI* cell = &m_cells[location.m_pointId];
        if (ProjectToCell(*cell, location.m_position, 100.0f)) {
            return true;
        }
        for (unsigned int index = 0; index < 3; index++) {
            const SCellAI* neighbour = cell->m_neighbours[index];
            if (neighbour != NULL && ProjectToCell(*neighbour, location.m_position, 100.0f)) {
                location.m_pointId = neighbour - m_cells.m_items;
                location.m_cellCenter = neighbour->m_center;
                return true;
            }
        }
    }
    int nearest = FindNearestCell(position);
    location.m_pointId = nearest;
    if (nearest == -1) {
        location.m_map = NULL;
    } else {
        location.m_map = const_cast<CGameAIMap_NavMesh*>(this);
    }
    return nearest != -1;
}

static void AddCellBorders(CGameAIMap_NavMesh::SCellAI* cell, ObstaclesContainer& obstacles)
{
    for (int index = 0; index < 3; index++) {
        CGameAIMap_NavMesh::SGateAI* gate = cell->m_borderGates[index];
        if (gate != NULL) {
            obstacles.AddStatic(gate->m_vertices[0]->m_position, gate->m_vertices[1]->m_position,
                                gate->m_inwardNormal, 0.0f);
        }
    }
}

void CGameAIMap_NavMesh::FindObstacles(const Location& location, ObstaclesContainer& obstacles)
{
    obstacles.Clear(false, true);
    if (location.m_pointId >= 0 && location.m_pointId < m_cells.size()) {
        SCellAI* cell = &m_cells[location.m_pointId];
        if (cell == NULL) {
            return;
        }
        AddCellBorders(cell, obstacles);
        for (int index = 0; index < 3; index++) {
            if (cell->m_neighbours[index] != NULL) {
                AddCellBorders(cell->m_neighbours[index], obstacles);
            }
        }
    }
}

void CGameAIMap_NavMesh::LocationFromId(Location& location, int pointId) const
{
    location.m_map = const_cast<CGameAIMap_NavMesh*>(this);
    location.m_position = GetMapPointPostition(pointId);
    if (pointId >= 0 && pointId < m_specialPoints.size()) {
        location.m_pointId = m_specialPoints[pointId].m_cell->m_id;
    } else {
        location.m_pointId = -1;
    }
    if (location.m_pointId <= -1) {
        location.m_map = NULL;
    }
    location.m_syncDistance = 0.0f;
}

bool CGameAIMap_NavMesh::FindPath(CSimplePath& result, const Location& start, const Location& end,
                                 float radius, unsigned int flags)
{
    CPath_NavMesh path;
    SCellAI* first = &m_cells[start.m_pointId];
    if (first == NULL) {
        return false;
    }
    SCellAI* last = &m_cells[end.m_pointId];
    if (last == NULL || !FindPathAStarImpl(path, first, last, radius, flags)) {
        return false;
    }
    ConstructPrecisePath(path, NULL);
    int pointCount = path.m_pointCount;
    if (pointCount > 0) {
        result.m_pointCount = pointCount;
        for (int index = 0; index < pointCount; index++) {
            CPath_NavMesh::SWayPoint* point = path.GetPathPoint(index);
            if (index == pointCount - 1) {
                result.m_points[index].m_pointId = start.m_pointId;
                result.m_points[index].m_position = start.m_position;
            } else {
                result.m_points[index].m_pointId = point->m_cellId;
                result.m_points[index].m_position = point->m_position;
            }
            result.m_points[index].m_distance = .1f;
        }
    } else {
        result.m_pointCount = 1;
    }
    result.m_points[0].m_pointId = end.m_pointId;
    result.m_points[0].m_position = end.m_position;
    result.m_points[0].m_distance = .1f;
    return true;
}

bool CGameAIMap_NavMesh::Pick(Location& location, int x, int y,
                             swerve::Camera<CSwerveAssertingErrorHandler>& camera) const
{
    // Original e5b6c actually creates/calls/releases, ignores the hit and always returns false; side effects must not be omitted.
    swerve::RayIntersection<CSwerveAssertingErrorHandler> intersection(CSwerve::GetInstance()->m_swerve);
    swerve::Group<CSwerveAssertingErrorHandler> root = m_navigationRoot;
    root.pick(-1, (float)x, (float)y, camera, intersection);
    return false;
}

int CGameAIMap_NavMesh::GetPointIDByName(const XString& name) const
{
    for (int index = 0; index < m_specialPoints.size(); index++) {
        if (m_specialPoints[index].m_name == name) {
            return m_specialPoints[index].m_id;
        }
    }
    return 0;
}

void CGameAIMap_NavMesh::RenderCellInformation(const SCellAI& cell)
{
    DGCamera* camera = ((App*)WindowApp::m_instance)->m_helper->m_swerveGame->m_camera;
    if (camera != NULL) {
        int x = 0;
        int y = 0;
        if (camera->GetScreenPosition(&x, &y, cell.m_center)) {
            CAuxRenderText::GetInstance()->AddTextInfoXY(x, y, L"%d", cell.m_id);
        }
    }
}

void CGameAIMap_NavMesh::RenderMap(CAuxRender* render, int mode)
{
    if (render == NULL) {
        return;
    }
    vec3 boxSize(.2f, .2f, .2f);
    for (int index = 0; index < m_gates.size(); index++) {
        SGateAI& gate = m_gates[index];
        render->DrawLine(gate.m_vertices[0]->m_position, gate.m_vertices[1]->m_position, .05f, 0xff7e9092);
    }
    for (int index = 0; index < m_borderGates.size(); index++) {
        SGateAI& gate = m_borderGates[index];
        render->DrawLine(gate.m_vertices[0]->m_position, gate.m_vertices[1]->m_position, .05f, 0xff7f0000);
        vec3 middle = (gate.m_vertices[0]->m_position + gate.m_vertices[1]->m_position) * .5f;
        vec3 end = middle + m_borderGates[index].m_inwardNormal;
        render->DrawLine(middle, end, .05f, 0xff7f0000);
    }
    for (int index = 0; index < m_specialPoints.size(); index++) {
        SSpecialPointAI* point = &m_specialPoints[index];
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
        if (mode == 0) {
            RenderPointInformation(*point);
        }
    }
    if (mode == 0) {
        for (int index = 0; index < m_cells.size(); index++) {
            RenderCellInformation(m_cells[index]);
        }
    } else if (mode == 2) {
        for (int index = 0; index < m_cells.size(); index++) {
            SCellAI& cell = m_cells[index];
            if (cell.m_parent != NULL) {
                render->DrawLine(cell.m_center, cell.m_parent->m_center, .05f, 0xffff9600);
            }
        }
    }
}

void CGameAIMap_NavMesh::FindSpecialPoints()
{
    int count = m_specialPoints.size();
    int regularCount = 0;
    int oneCount = 0;
    int twoCount = 0;
    int cameraCount = 0;
    for (int index = 0; index < count; index++) {
        const SSpecialPointAI& point = m_specialPoints[index];
        if (point.m_pointType == 0) {
            regularCount++;
        } else if (point.m_pointType == 1) {
            oneCount++;
        } else if (point.m_pointType == 2) {
            twoCount++;
        }
        if (point.m_cameraAttitudes.size() > cameraCount) {
            cameraCount = point.m_cameraAttitudes.size();
        }
    }
    m_cameraPoints.ensureCapacity(cameraCount);
    m_regularPoints.ensureCapacity(regularCount);
    m_typeOnePoints.ensureCapacity(oneCount);
    m_typeTwoPoints.ensureCapacity(twoCount);
    for (int index = 0; index < count; index++) {
        switch (m_specialPoints[index].m_pointType) {
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
    int level = CBH_Player::GetInstance()->m_id;
    for (int index = 0; index < cameraCount; index++) {
        m_cameraPoints.addElement(Vector<int>());
    }
    for (int index = 0; index < count; index++) {
        SSpecialPointAI& point = m_specialPoints[index];
        if (point.m_pointType == 0) {
            for (int camera = 0; camera < point.m_cameraAttitudes.size(); camera++) {
                if (point.m_cameraAttitudes[camera].m_startLevel > level) {
                    point.m_cameraAttitudes[camera].m_visible = false;
                }
                if (point.m_cameraAttitudes[camera].m_visible) {
                    m_cameraPoints[camera].addElement(index);
                }
            }
        }
    }
}

bool CGameAIMap_NavMesh::CreateNavigationMeshVertices(swerve::World<CSwerveAssertingErrorHandler>& world)
{
    if (world.m_interface == NULL) {
        return false;
    }
    swerve::Group<CSwerveAssertingErrorHandler> root(world.find(DGHelper::getSwerveID("navigation_meshTransform")));
    if (root.m_interface == NULL) {
        return false;
    }
    swerve::Mesh<CSwerveAssertingErrorHandler> mesh(world.find(DGHelper::getSwerveID("navigation_mesh")));
    if (mesh.m_interface == NULL) {
        return false;
    }
    m_navigationRoot.swerve::QueryInterface<CSwerveAssertingErrorHandler>::operator=(root.m_interface);
    swerve::Transform<CSwerveAssertingErrorHandler> transform(CSwerve::GetInstance()->m_swerve);
    mesh.getTransformTo(((App*)WindowApp::m_instance)->m_helper->m_swerveGame->m_cameraNode, transform);
    swerve::VertexBuffer<CSwerveAssertingErrorHandler> buffer = mesh.getVertexBuffer();
    if (buffer.m_interface == NULL) {
        return false;
    }
    float scaleBias[4];
    swerve::VertexArray<CSwerveAssertingErrorHandler> positions = buffer.getPositions(scaleBias);
    if (positions.m_interface == NULL) {
        return false;
    }
    int count = positions.getVertexCount();
    m_vertices.ensureCapacity(count);
    short* coordinates = new short[count * 3];
    positions.get(0, count, count * 3, coordinates);
    for (int index = 0; index < count; index++) {
        // Original e4244 first reads all three shorts, then computes the homogeneous components, and finally writes the transform input.
        SPointAI point;
        short x = coordinates[index * 3];
        short y = coordinates[index * 3 + 1];
        short z = coordinates[index * 3 + 2];
        float positionX = (float)x * scaleBias[0] + scaleBias[1];
        float positionY = (float)y * scaleBias[0] + scaleBias[2];
        float positionZ = (float)z * scaleBias[0] + scaleBias[3];
        float values[4] = { positionX, positionY, positionZ, 1.0f };
        transform.transform(4, values);
        point.m_position.m_x = values[0];
        point.m_position.m_y = values[1];
        point.m_position.m_z = values[2];
        point.m_border = false;
        m_vertices.addElement(point);
    }
    delete[] coordinates;
    return true;
}

bool CGameAIMap_NavMesh::CreateNavigationMeshCells(swerve::World<CSwerveAssertingErrorHandler>& world)
{
    if (world.m_interface == NULL) {
        return false;
    }
    swerve::Mesh<CSwerveAssertingErrorHandler> mesh(world.find(DGHelper::getSwerveID("navigation_mesh")));
    if (mesh.m_interface == NULL) {
        return false;
    }
    mesh.setRenderingEnable(false);
    mesh.setPickingEnable(false);
    Vector<int> mapping;
    int vertexCount = m_vertices.size();
    mapping.ensureCapacity(vertexCount);
    for (int index = 0; index < vertexCount; index++) {
        mapping.addElement(index);
    }
    for (int first = 0; first < vertexCount; first++) {
        for (int second = 0; second < vertexCount; second++) {
            if (first != second && mapping[first] == first && IsPointTheSame(&m_vertices[first], &m_vertices[second])) {
                mapping[second] = first;
            }
        }
    }
    int submeshCount = mesh.getSubmeshCount();
    int totalIndices = 0;
    for (int index = 0; index < submeshCount; index++) {
        swerve::IndexBuffer<CSwerveAssertingErrorHandler> buffer = mesh.getTriangleIndexBuffer(index);
        if (buffer.m_interface != NULL) {
            totalIndices += buffer.getIndexCount();
        }
    }
    m_openList.reserve(totalIndices / 3);
    m_cells.ensureCapacity(totalIndices / 3);
    for (int submesh = 0; submesh < submeshCount; submesh++) {
        swerve::IndexBuffer<CSwerveAssertingErrorHandler> buffer = mesh.getTriangleIndexBuffer(submesh);
        if (buffer.m_interface == NULL) {
            continue;
        }
        int indexCount = buffer.getIndexCount();
        if (indexCount % 3 != 0) {
            continue;
        }
        unsigned int* indices = new unsigned int[indexCount];
        buffer.getIndices(indexCount, indices);
        for (int triangle = 0; triangle < indexCount / 3; triangle++) {
            int first = indices[triangle * 3];
            int second = indices[triangle * 3 + 1];
            int third = indices[triangle * 3 + 2];
            if (first < 0 || first == second || first == third || second == third ||
                first >= m_vertices.size() || second < 0 || second >= m_vertices.size() ||
                third < 0 || third >= m_vertices.size()) {
                continue;
            }
            // Original e45b8 only initializes the two states, vec3 and Vector; other undefined scalars are not zero-filled.
            SCellAI cell;
            cell.m_state0 = 0;
            cell.m_state4 = 0;
            cell.m_id = m_cells.size();
            cell.m_submeshIndex = submesh;
            cell.m_vertexIds[0] = mapping[first];
            cell.m_vertexIds[1] = mapping[second];
            cell.m_vertexIds[2] = mapping[third];
            for (int vertex = 0; vertex < 3; vertex++) {
                cell.m_vertices[vertex] = &m_vertices[cell.m_vertexIds[vertex]];
            }
            for (int vertex = 0; vertex < 3; vertex++) {
                cell.m_center = cell.m_center + cell.m_vertices[vertex]->m_position;
            }
            cell.m_center.m_x = cell.m_center.m_x / 3.0f;
            cell.m_center.m_y = cell.m_center.m_y / 3.0f;
            cell.m_center.m_z = cell.m_center.m_z / 3.0f;
            m_cells.addElement(cell);
        }
        delete[] indices;
    }
    return true;
}

bool CGameAIMap_NavMesh::CreateNavigationMeshGates()
{
    // Original e46c8 reads the cell count before building the temporary edge table.
    int cellCount = m_cells.size();
    Vector<CPoint> edges;
    edges.ensureCapacity(cellCount * 3);
    for (int first = 0; first < cellCount; first++) {
        for (int second = 0; second < cellCount; second++) {
            if (first == second) {
                continue;
            }
            bool shared[3];
            int sharedCount = 0;
            for (int vertex = 0; vertex < 3; vertex++) {
                int id = m_cells[first].m_vertexIds[vertex];
                if (id == m_cells[second].m_vertexIds[0] ||
                    id == m_cells[second].m_vertexIds[1] ||
                    id == m_cells[second].m_vertexIds[2]) {
                    shared[vertex] = true;
                    sharedCount++;
                } else {
                    shared[vertex] = false;
                }
            }
            if (sharedCount == 2) {
                CPoint edge(-1, -1);
                for (int vertex = 0; vertex < 3; vertex++) {
                    if (shared[vertex]) {
                        if (edge.m_x == -1) {
                            edge.m_x = m_cells[first].m_vertexIds[vertex];
                        } else {
                            edge.m_y = m_cells[first].m_vertexIds[vertex];
                        }
                    }
                }
                if (edge.m_x != edge.m_y) {
                    bool exists = false;
                    for (int index = 0; index < edges.size(); index++) {
                        if ((edges[index].m_x == edge.m_x && edges[index].m_y == edge.m_y) ||
                            (edges[index].m_x == edge.m_y && edges[index].m_y == edge.m_x)) {
                            exists = true;
                            break;
                        }
                    }
                    if (!exists) {
                        edges.addElement(edge);
                    }
                }
            }
        }
    }
    int count = edges.size();
    if (count > 0) {
        m_gates.ensureCapacity(count);
        for (int index = 0; index < count; index++) {
            SPointAI* first = &m_vertices[edges[index].m_x];
            SPointAI* second = &m_vertices[edges[index].m_y];
            if (first != NULL && second != NULL) {
                SGateAI gate;
                gate.m_vertices[0] = first;
                gate.m_vertices[1] = second;
                gate.m_id = index;
                m_gates.addElement(gate);
            }
        }
    }
    return true;
}

bool CGameAIMap_NavMesh::CreateNavigationMeshCohesion()
{
    int count = m_cells.size();
    int borderCount = 0;
    for (int index = 0; index < count; index++) {
        for (int edge = 0; edge < 3; edge++) {
            SCellAI& cell = m_cells[index];
            cell.m_gates[edge] = FindGate(cell.m_vertices[edge], cell.m_vertices[(edge + 1) % 3]);
            if (cell.m_gates[edge] == NULL) {
                borderCount++;
            }
        }
    }
    m_borderGates.ensureCapacity(borderCount);
    for (int index = 0; index < count; index++) {
        SCellAI& cell = m_cells[index];
        for (int edge = 0; edge < 3; edge++) {
            SGateAI* border = NULL;
            if (cell.m_gates[edge] == NULL) {
                SGateAI gate;
                gate.m_vertices[0] = cell.m_vertices[edge];
                gate.m_vertices[1] = cell.m_vertices[(edge + 1) % 3];
                vec3 normal = MathLib::DistanceToLine(cell.m_vertices[(edge + 2) % 3]->m_position,
                                       gate.m_vertices[0]->m_position, gate.m_vertices[1]->m_position);
                float inverseLength = MathLib::InvSqrt(normal.LengthSqr());
                gate.m_inwardNormal = normal * inverseLength;
                m_borderGates.addElement(gate);
                border = &m_borderGates[m_borderGates.size() - 1];
            }
            cell.m_borderGates[edge] = border;
        }
    }
    for (int index = 0; index < borderCount; index++) {
        m_borderGates[index].m_vertices[0]->m_border = true;
        m_borderGates[index].m_vertices[1]->m_border = true;
    }
    for (int index = 0; index < count; index++) {
        SCellAI* cell = &m_cells[index];
        for (int edge = 0; edge < 3; edge++) {
            if (cell->m_gates[edge] == NULL) {
                cell->m_neighbours[edge] = NULL;
            } else {
                SCellAI* neighbour = FindNeighbour(cell, cell->m_gates[edge]);
                cell->m_neighbours[edge] = neighbour;
                if (neighbour != NULL) {
                    vec3 difference = cell->m_center - neighbour->m_center;
                    float squared = difference.LengthSqr();
                    cell->m_gates[edge]->m_distance = MathLib::InvSqrt(squared) * squared;
                    // The same shared gate calls the random generator separately from each end; the overwrite and call count are preserved.
                    cell->m_gates[edge]->m_randomValue = com::glu::platform::core::CRandGen::GetInstance()->Generate();
                }
            }
        }
    }
    return true;
}

bool BelongsToCell(const CGameAIMap_NavMesh::SCellAI& cell, const vec3& position, float length, float& distance)
{
    vec3 first = cell.m_vertices[0]->m_position;
    vec3 second = cell.m_vertices[1]->m_position;
    vec3 third = cell.m_vertices[2]->m_position;
    Plane plane(first, second, third, true);
    third.m_z = 0.0f;
    second.m_z = 0.0f;
    first.m_z = 0.0f;
    distance = length;
    vec3 hit(0, 0, 0);
    if (MathLib::RayTriangleIntersection(position, vec3(0, 0, -1), length, plane,
                                        first, second, third, hit, distance, .1f, .1f, .1f, .1f)) {
        return true;
    }
    return MathLib::RayTriangleIntersection(position, vec3(0, 0, 1), length, plane,
                                            first, second, third, hit, distance, .1f, .1f, .1f, .1f);
}

bool ProjectToCell(const CGameAIMap_NavMesh::SCellAI& cell, vec3& position, float length)
{
    vec3 first = cell.m_vertices[0]->m_position;
    vec3 second = cell.m_vertices[1]->m_position;
    vec3 third = cell.m_vertices[2]->m_position;
    Plane plane(first, second, third, true);
    third.m_z = 0.0f;
    second.m_z = 0.0f;
    first.m_z = 0.0f;
    float distance = length;
    vec3 hit(0, 0, 0);
    bool found = MathLib::RayTriangleIntersection(position, vec3(0, 0, -1), length, plane,
                         first, second, third, hit, distance, .1f, .1f, .1f, .1f);
    if (!found) {
        found = MathLib::RayTriangleIntersection(position, vec3(0, 0, 1), length, plane,
                         first, second, third, hit, distance, .1f, .1f, .1f, .1f);
        if (!found) {
            return false;
        }
    }
    position = plane.Projection(position);
    return true;
}

bool CGameAIMap_NavMesh::Init(swerve::World<CSwerveAssertingErrorHandler>& world)
{
    if (!LoadNavigationMesh(world)) {
        return false;
    }
    if (!LoadSpecialPoint(world)) {
        return false;
    }
    BindPointToCell(false);
    FindSpecialPoints();
    return true;
}

bool CGameAIMap_NavMesh::LoadNavigationMesh(swerve::World<CSwerveAssertingErrorHandler>& world)
{
    if (!CreateNavigationMeshVertices(world)) {
        return false;
    }
    if (!CreateNavigationMeshCells(world)) {
        return false;
    }
    CreateNavigationMeshGates();
    CreateNavigationMeshCohesion();
    return true;
}

bool CGameAIMap_NavMesh::LoadSpecialPoint(swerve::World<CSwerveAssertingErrorHandler>& world)
{
    if (world.m_interface == NULL) {
        return false;
    }
    swerve::Group<CSwerveAssertingErrorHandler> root(world.find(DGHelper::getSwerveID("AI_RootTransform")));
    if (root.m_interface == NULL) {
        return false;
    }
    int count = root.getChildCount();
    m_specialPoints.ensureCapacity(count);
    for (int index = 0; index < count; index++) {
        swerve::Node<CSwerveAssertingErrorHandler> node = root.getChild(index);
        if (node.m_interface == NULL) {
            return false;
        }
        SSpecialPointAI point;
        m_specialPoints.addElement(point);
        SSpecialPointAI* destination = &m_specialPoints[m_specialPoints.size() - 1];
        float translation[3];
        node.getTranslation(translation);
        // From original e3a9c, copied verbatim with integer ldr/str: position assigned component by component, orientation copied whole as a 16-byte struct.
        destination->m_position.m_x = translation[0];
        destination->m_position.m_y = translation[1];
        destination->m_position.m_z = translation[2];
        float orientation[4];
        node.getOrientation(orientation);
        *(Quaternion*)destination->m_orientation = *(const Quaternion*)orientation;
        destination->m_id = index;
        DefineCameraCharacteristics(destination, node);
    }
    return true;
}

void CGameAIMap_NavMesh::BindPointToCell(bool project)
{
    for (int index = 0; index < m_specialPoints.size(); index++) {
        int cellId = FindNearestCell(m_specialPoints[index].m_position);
        if (cellId < 0) {
            m_specialPoints[index].m_cell = NULL;
        } else {
            if (project) {
                m_specialPoints[index].m_position = PutPointToCell(m_cells[cellId], m_specialPoints[index].m_position);
            }
            m_specialPoints[index].m_cell = &m_cells[cellId];
        }
    }
    for (int cellIndex = 0; cellIndex < m_cells.size(); cellIndex++) {
        int count = 0;
        for (int index = 0; index < m_specialPoints.size(); index++) {
            if (m_specialPoints[index].m_cell == &m_cells[cellIndex]) {
                count++;
            }
        }
        SCellAI* cell = &m_cells[cellIndex];
        cell->m_specialPoints.ensureCapacity(count);
        for (int index = 0; index < m_specialPoints.size(); index++) {
            if (m_specialPoints[index].m_cell == cell) {
                m_cells[cellIndex].m_specialPoints.addElement(&m_specialPoints[index]);
            }
        }
    }
}

bool CGameAIMap_NavMesh::FindAnotherPoint(int* pointId, int previousPointId) const
{
    int count = m_specialPoints.size();
    if (count < 2 || previousPointId < 0 || previousPointId >= count) {
        return false;
    }
    int attempts = 0;
    bool found = false;
    do {
        int candidate = GetPointIDImpl(m_regularPoints);
        if (candidate != previousPointId) {
            *pointId = candidate;
            found = true;
        }
        attempts++;
    } while (attempts < count || !found);
    return true;
}

bool CGameAIMap_NavMesh::IsPointTheSame(const SPointAI* first, const SPointAI* second)
{
    if (first == NULL || second == NULL) {
        return false;
    }
    vec3 difference = first->m_position - second->m_position;
    float squared = difference.LengthSqr();
    return MathLib::InvSqrt(squared) * squared < SAME_POINT_THRESHOLD;
}

bool CGameAIMap_NavMesh::IsPositionTheSame(const vec3& first, const vec3& second)
{
    SPointAI firstPoint;
    firstPoint.m_position = first;
    SPointAI secondPoint;
    secondPoint.m_position = second;
    return IsPointTheSame(&firstPoint, &secondPoint);
}

CGameAIMap_NavMesh::SGateAI* CGameAIMap_NavMesh::FindGate(const SPointAI* first, const SPointAI* second)
{
    if (first == NULL || second == NULL) {
        return NULL;
    }
    for (int index = 0; index < m_gates.size(); index++) {
        SGateAI* gate = &m_gates[index];
        if ((gate->m_vertices[0] == first && gate->m_vertices[1] == second) ||
            (gate->m_vertices[1] == first && gate->m_vertices[0] == second)) {
            return gate;
        }
    }
    return NULL;
}

CGameAIMap_NavMesh::SCellAI* CGameAIMap_NavMesh::FindNeighbour(const SCellAI* cell, const SGateAI* gate)
{
    if (cell == NULL || gate == NULL) {
        return NULL;
    }
    for (int index = 0; index < m_cells.size(); index++) {
        SCellAI* candidate = &m_cells[index];
        if (candidate != cell) {
            for (int edge = 0; edge < 3; edge++) {
                if (candidate->m_gates[edge] == gate) {
                    return candidate;
                }
            }
        }
    }
    return NULL;
}

vec3 CGameAIMap_NavMesh::PutPointToCell(const SCellAI& cell, const vec3& position) const
{
    const vec3& first = cell.m_vertices[0]->m_position;
    Plane plane(first, cell.m_vertices[1]->m_position, cell.m_vertices[2]->m_position, true);
    vec3 result = position;
    if (plane.m_normal.m_z != 0.0f) {
        result.m_z = ((first.m_x - position.m_x) * plane.m_normal.m_x +
                      (first.m_y - position.m_y) * plane.m_normal.m_y) / plane.m_normal.m_z + first.m_z;
    }
    return result;
}

int CGameAIMap_NavMesh::GetPointIDImpl(const Vector<int>& points) const
{
    int count = points.size();
    if (m_specialPoints.size() < 1 || count < 1) {
        return -1;
    }
    int index = com::glu::platform::core::CRandGen::GetInstance()->GetRandRange(0, count - 1);
    return points[index];
}


const vec3& CGameAIMap_NavMesh::GetMapPointPostition(int pointId) const
{
    return m_specialPoints[pointId].m_position;
}


const float* CGameAIMap_NavMesh::GetMapPointOrientation(int pointId) const
{
    return m_specialPoints[pointId].m_orientation;
}


int CGameAIMap_NavMesh::GetHideBehaviour(int pointId, int cameraId) const
{
    return m_specialPoints[pointId].m_cameraAttitudes[cameraId].m_safetyType;
}


void CGameAIMap_NavMesh::RenderPointInformation(const SSpecialPointAI& point)
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


int CGameAIMap_NavMesh::GetPointSpawnType(int pointId) const
{
    for (int index = 0; index < m_specialPoints.size(); index++) {
        if (m_specialPoints[index].m_id == pointId) {
            return m_specialPoints[index].m_spawnType;
        }
    }
    XString message = XString::Format(L"Can't find map point with id \"%d\"", pointId);
    return 0;
}

void CGameAIMap_NavMesh::DefineCameraCharacteristics(SSpecialPointAI* point, swerve::Node<CSwerveAssertingErrorHandler>& node)
{
    char parameterName[1000];
    int pointType;
    int spawnType;
    com::glu::platform::core::ICStdUtil::SPrintF(parameterName, "PointType");
    ((App*)WindowApp::m_instance)->m_helper->getUserParameter(node, parameterName, &pointType);
    // Original e4b5c/e4b92 only clamps the final member and does not write back getUserParameter's stack out-parameter.
    if (pointType < 0) {
        point->m_pointType = 0;
    } else {
        point->m_pointType = pointType;
    }
    com::glu::platform::core::ICStdUtil::SPrintF(parameterName, "SpawnType");
    ((App*)WindowApp::m_instance)->m_helper->getUserParameter(node, parameterName, &spawnType);
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
    point->m_state3c = 0;
    // Side effect of the ID query at original e4c1a is preserved; the value does not overwrite the loop index m_id.
    int ignoredId;
    com::glu::platform::core::ICStdUtil::SPrintF(parameterName, "ID");
    ((App*)WindowApp::m_instance)->m_helper->getUserParameter(node, parameterName, &ignoredId);
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

void CGameAIMap_NavMesh::AStarInit()
{
    int pointCount = m_cells.size();
    for (int index = 0; index < pointCount; index++) {
        SCellAI& point = m_cells[index];
        point.m_pathDistance = 0.0f;
        point.m_heuristicDistance = 0.0f;
        point.m_parentEdgeDistance = 0.0f;
        point.m_parent = NULL;
        point.m_searchState = 0;
    }
    m_openList.m_size = 0;
}

void CGameAIMap_NavMesh::OpenCellListPushBack(SCellAI* point, SCellAI* end, SCellAI* parent, float edgeDistance)
{
    vec3 difference = end->m_center - point->m_center;
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

CGameAIMap_NavMesh::SCellAI* CGameAIMap_NavMesh::OpenCellListPop()
{
    SCellAI* bestPoint = NULL;
    int count = m_openList.m_size;
    if (count > 0) {
        int bestIndex = 0;
        bestPoint = m_openList.m_data[0];
        if (count >= 2) {
            float bestCost = bestPoint->m_pathDistance + bestPoint->m_heuristicDistance;
            for (int index = 1; index < count; index++) {
                SCellAI* candidate = m_openList.m_data[index];
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
