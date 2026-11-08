// CGameAIMap.h - original u412 map interface and graph node owner; no virtual destructor, the original vtable has exactly fourteen slots.
#ifndef CGAMEAIMAP_H
#define CGAMEAIMAP_H

#include "MathLib.h"
#include "Vector_ss.h"
#include "CFixedVector.h"
#include "xString.h"
#include "tws.h"

class CAuxRender;
class CPath;
class ObstaclesContainer;

/** Simplified path the map provides to units; the float/id order of points is the reverse of the internal CPath. */
class CSimplePath {
public:
    struct SPathPoint {
        vec3 m_position;
        float m_distance;  // +0x0c, both map FindPath implementations write 0.1f
        int m_pointId;     // +0x10
    };

    SPathPoint m_points[256];
    int m_pointCount;  // +0x1400
};

/** 14-slot non-owning interface; the original GetMapPointPostition keeps its symbol spelling. */
class IGameAIMap {
public:
    struct Location {
        Location() {}
        // ea21c..ea23e copies as two 3-component vectors plus three scalars; e8f90 uses an indirect address for the by-value parameter.
        // Explicit memberwise copy preserves this non-trivial value ABI; the original syntax is not uniquely determinable, no destructor side effects added.
        Location(const Location& other)
            : m_position(other.m_position), m_cellCenter(other.m_cellCenter),
              m_map(other.m_map), m_pointId(other.m_pointId),
              m_syncDistance(other.m_syncDistance) {}

        vec3 m_position;       // +0x00
        vec3 m_cellCenter;     // +0x0c, Nav SyncLocation copies the three-vertex centroid from cell+0x28
        IGameAIMap* m_map;     // +0x18, const SyncLocation stores the original this address
        int m_pointId;         // +0x1c
        float m_syncDistance; // +0x20
    };

    virtual bool Init(swerve::World<CSwerveAssertingErrorHandler>& world) = 0;
    virtual bool SyncLocation(Location& location, const vec3& position) const = 0;
    virtual void FindObstacles(const Location& location, ObstaclesContainer& obstacles) = 0;
    virtual void LocationFromId(Location& location, int pointId) const = 0;
    virtual void LocationFromName(Location& location, const XString& name);
    virtual bool FindPath(CSimplePath& path, const Location& start, const Location& end,
                          float radius, unsigned int flags) = 0;
    virtual const vec3& GetMapPointPostition(int pointId) const = 0;
    virtual const float* GetMapPointOrientation(int pointId) const = 0;
    virtual int GetPointIDByName(const XString& name) const = 0;
    virtual bool FindAnotherPoint(int* pointId, int previousPointId) const = 0;
    virtual bool Pick(Location& location, int x, int y, swerve::Camera<CSwerveAssertingErrorHandler>& camera) const = 0;
    virtual int GetPointSpawnType(int pointId) const = 0;
    virtual int GetHideBehaviour(int pointId, int cameraId) const = 0;
    virtual void RenderMap(CAuxRender* render, int drawRadii) = 0;
};

/** Node graph of the scene AI_RootTransform, camera visibility lists and the A* open list. */
class CGameAIMap : public IGameAIMap {
public:
    struct SGameAIPoint;
    struct SGameAIEdge {
        SGameAIPoint* m_target;
        float m_distance;
    };
    struct SCameraAttitude {
        int m_safetyType;
        int m_startLevel;
        bool m_visible;
        bool m_safe;
    };
    struct SGameAIPoint {
        Vector<SGameAIEdge> m_edges;  // +0x00
        vec3 m_position;             // +0x10
        float m_orientation[4];     // +0x1c
        int m_id;                   // +0x2c
        float m_pathDistance;       // +0x30
        float m_heuristicDistance;  // +0x34
        float m_parentEdgeDistance; // +0x38
        SGameAIPoint* m_parent;     // +0x3c
        int m_searchState;          // +0x40: 0 unvisited, 1 open, 2 closed
        int m_state44;              // Kept by the original copy; this unit neither assigns nor reads its meaning
        int m_spawnType;            // +0x48
        int m_pointType;            // +0x4c
        Vector<SCameraAttitude> m_cameraAttitudes; // +0x50
        int m_state60;              // Zeroed by DefineGameCharacteristic
        XString m_name;             // +0x64
        float m_radius;             // +0x68
        bool m_usable;              // +0x6c
    };

    virtual bool Init(swerve::World<CSwerveAssertingErrorHandler>& world);
    virtual bool SyncLocation(Location& location, const vec3& position) const;
    virtual void FindObstacles(const Location& location, ObstaclesContainer& obstacles);
    virtual void LocationFromId(Location& location, int pointId) const;
    virtual bool FindPath(CSimplePath& path, const Location& start, const Location& end,
                          float radius, unsigned int flags);
    virtual const vec3& GetMapPointPostition(int pointId) const;
    virtual const float* GetMapPointOrientation(int pointId) const;
    virtual int GetPointIDByName(const XString& name) const;
    virtual bool FindAnotherPoint(int* pointId, int previousPointId) const;
    virtual bool Pick(Location& location, int x, int y, swerve::Camera<CSwerveAssertingErrorHandler>& camera) const;
    virtual int GetPointSpawnType(int pointId) const;
    virtual int GetHideBehaviour(int pointId, int cameraId) const;
    virtual void RenderMap(CAuxRender* render, int drawRadii);

    bool FindNearestPoint(int* pointId, float* distance, const vec3& position) const;
    void DefineGameCharacteristic(SGameAIPoint* point, swerve::Node<CSwerveAssertingErrorHandler>& node);
    void FindSpecialPoints();
    bool FindPathAStarImpl(CPath* path, SGameAIPoint* start, SGameAIPoint* end);
    int GetPointIDImpl(const Vector<int>& points) const;
    void ConstructPath(CPath* path, SGameAIPoint* point);
    void InitializeAStar();
    void OpenListPushBack(SGameAIPoint* point, SGameAIPoint* end, SGameAIPoint* parent, float edgeDistance);
    SGameAIPoint* OpenListPop();
    void RenderPointInformation(const SGameAIPoint& point);

    Vector<SGameAIPoint> m_points;         // +0x04
    Vector<int> m_regularPoints;           // +0x14
    Vector<int> m_typeOnePoints;           // +0x24
    Vector<int> m_typeTwoPoints;           // +0x34
    Vector<Vector<int> > m_cameraPoints;   // +0x44
    CFixedVector<SGameAIPoint*> m_openList; // +0x54
};

#endif
