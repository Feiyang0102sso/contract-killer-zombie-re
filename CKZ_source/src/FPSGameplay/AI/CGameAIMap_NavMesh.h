// Original u413 navigation mesh owner. Fourteen-slot interface, no virtual destructor; unknown state covered by the original copy keeps offset names.
#ifndef CGAMEAIMAP_NAVMESH_H
#define CGAMEAIMAP_NAVMESH_H
#include "CGameAIMap.h"
class CPath_NavMesh;
class CGameAIMap_NavMesh : public IGameAIMap {
public:
    struct SCellAI;
    struct SPointAI {
        vec3 m_position;
        bool m_border;
        int m_state10;
        int m_state14;
        int m_state18[3]; // e5c64 original copy covers the full nine words; these tail states are not initialized in this unit.
    };
    struct SGateAI {
        int m_id;
        SPointAI* m_vertices[2];
        float m_distance;
        vec3 m_inwardNormal;
        unsigned int m_randomValue;
    };
    struct SCameraAttitude {
        int m_safetyType;
        int m_startLevel;
        bool m_visible;
        bool m_safe;
    };
    struct SSpecialPointAI {
        // The normal copies at e63d0/e65cc do not zero the whole block first; the user constructor only runs default constructors of non-trivial members.
        SSpecialPointAI() : m_cell(NULL), m_state3c(0) {}  // e3a6a: the local point in LoadSpecialPoint zeroes +0x38/+0x3c
        int m_id;
        Vector<SCameraAttitude> m_cameraAttitudes;
        vec3 m_position;
        float m_orientation[4];
        int m_pointType;
        int m_spawnType;
        SCellAI* m_cell;
        int m_state3c;
        XString m_name;
        float m_radius;
        bool m_flag48;
    };
    struct SCellAI {
        // e5ea8/e6750 likewise have no implicit whole-block zeroing; the two start states are set explicitly by CreateCells.
        SCellAI() {}
        int m_state0;
        int m_state4;
        int m_id;
        int m_submeshIndex;
        int m_vertexIds[3];
        SPointAI* m_vertices[3];
        vec3 m_center;
        SGateAI* m_gates[3];
        SGateAI* m_borderGates[3];
        SCellAI* m_neighbours[3];
        Vector<SSpecialPointAI*> m_specialPoints;
        float m_parentEdgeDistance;
        SCellAI* m_parent;
        int m_searchState;
        float m_pathDistance;
        float m_heuristicDistance;
    };
    virtual bool Init(swerve::World<CSwerveAssertingErrorHandler>& world);
    virtual bool SyncLocation(Location& location, const vec3& position) const;
    virtual void FindObstacles(const Location& location, ObstaclesContainer& obstacles);
    virtual void LocationFromId(Location& location, int pointId) const;
    virtual bool FindPath(CSimplePath& path, const Location& start, const Location& end, float radius, unsigned int flags);
    virtual const vec3& GetMapPointPostition(int pointId) const;
    virtual const float* GetMapPointOrientation(int pointId) const;
    virtual int GetPointIDByName(const XString& name) const;
    virtual bool FindAnotherPoint(int* pointId, int previousPointId) const;
    virtual bool Pick(Location& location, int x, int y, swerve::Camera<CSwerveAssertingErrorHandler>& camera) const;
    virtual int GetPointSpawnType(int pointId) const;
    virtual int GetHideBehaviour(int pointId, int cameraId) const;
    virtual void RenderMap(CAuxRender* render, int mode);
    bool LoadNavigationMesh(swerve::World<CSwerveAssertingErrorHandler>& world);
    bool LoadSpecialPoint(swerve::World<CSwerveAssertingErrorHandler>& world);
    void BindPointToCell(bool project);
    void FindSpecialPoints();
    int GetPointIDImpl(const Vector<int>& points) const;
    static bool IsPointTheSame(const SPointAI* first, const SPointAI* second);
    static bool IsPositionTheSame(const vec3& first, const vec3& second);
    SGateAI* FindGate(const SPointAI* first, const SPointAI* second);
    SCellAI* FindNeighbour(const SCellAI* cell, const SGateAI* gate);
    vec3 PutPointToCell(const SCellAI& cell, const vec3& position) const;
    bool CreateNavigationMeshVertices(swerve::World<CSwerveAssertingErrorHandler>& world);
    bool CreateNavigationMeshCells(swerve::World<CSwerveAssertingErrorHandler>& world);
    bool CreateNavigationMeshGates();
    bool CreateNavigationMeshCohesion();
    void DefineCameraCharacteristics(SSpecialPointAI* point, swerve::Node<CSwerveAssertingErrorHandler>& node);
    int FindNearestCell(const vec3& position) const;
    void DefineGatesForPath(CPath_NavMesh& path);
    void SmoothPath(CPath_NavMesh& path);
    void ConstructPrecisePath(CPath_NavMesh& path, SCellAI* start);
    void AStarInit();
    void OpenCellListPushBack(SCellAI* cell, SCellAI* end, SCellAI* parent, float edgeDistance);
    SCellAI* OpenCellListPop();
    bool FindPathAStarImpl(CPath_NavMesh& path, SCellAI* start, SCellAI* end, float radius, unsigned int flags);
    void RenderPointInformation(const SSpecialPointAI& point);
    void RenderCellInformation(const SCellAI& cell);
    static const float SAME_POINT_THRESHOLD;
    Vector<SPointAI> m_vertices; // +4
    Vector<SGateAI> m_gates; // +14
    Vector<SGateAI> m_borderGates; // +24
    Vector<SCellAI> m_cells; // +34
    Vector<SSpecialPointAI> m_specialPoints; // +44
    CFixedVector<SCellAI*> m_openList; // +54
    Vector<int> m_regularPoints; // +60
    Vector<int> m_typeOnePoints; // +70
    Vector<int> m_typeTwoPoints; // +80
    Vector<Vector<int> > m_cameraPoints; // +90
    swerve::Group<CSwerveAssertingErrorHandler> m_navigationRoot; // +a0
};
#endif
