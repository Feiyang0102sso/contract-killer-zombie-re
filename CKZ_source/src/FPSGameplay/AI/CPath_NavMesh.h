// CPath_NavMesh.h - original u415 three-consecutive-point gate-edge smoothing; Render is the only real virtual slot.
#ifndef CPATH_NAVMESH_H
#define CPATH_NAVMESH_H

#include "MathLib.h"

class CAuxRender;
class CUnitBody;

class CPath_NavMesh {
public:
    struct SWayPoint {
        vec3 m_position;
        vec3 m_gateEnds[2];
        int m_cellId;   // +0x24
        bool m_changed; // +0x28
    };
    struct SSmoothPoint {
        SWayPoint* m_previous;
        SWayPoint* m_current;
        SWayPoint* m_next;
        float m_angleCoef;

        void CalcAngleCoef();
    };

    CPath_NavMesh();
    void Clear();
    bool AddCell(int cellId);
    int GetCellID(int index) const;
    bool AddWayPoint(const vec3& position, int cellId);
    bool AddGateWayPoint(const vec3& position, int cellId, const vec3& firstEnd, const vec3& secondEnd);
    bool SmoothPoint(SSmoothPoint* point);
    void CreateSmoothInfo();
    void Smooth();
    SWayPoint* GetPathPoint(int index);
    virtual void Render(CAuxRender* render);

    SWayPoint m_points[256];        // +0x0004
    int m_pointCount;               // +0x2c04
    SSmoothPoint m_smoothPoints[256]; // +0x2c08
    int m_smoothCount;              // +0x3c08
    int m_state3c0c;                // Original constructor tail state, not read in the current owned range
    int m_state3c10;
    bool m_flag3c14;
    bool m_flag3c15;
    CUnitBody* m_body;               // +0x3c18, DefineGatesForPath takes GetRadius() at e4eae
    int m_cells[256];               // +0x3c1c
    int m_cellCount;                // +0x401c
    int m_lastCell;                 // +0x4020
};

#endif
