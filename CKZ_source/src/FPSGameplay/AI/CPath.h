// CPath.h - A* internal reversed path; 20B point layout of original u414 and both map FindPath implementations.
#ifndef CPATH_H
#define CPATH_H

#include "MathLib.h"

/** 256 internal path points with random radius offsets; not to be mixed with CSimplePath output points. */
class CPath {
public:
    struct SPathPoint {
        vec3 m_position;       // +0x00
        int m_pointId;         // +0x0c
        float m_edgeDistance;  // +0x10
    };

    CPath();
    void Clear();
    bool AddPointInRadius(const vec3& position, int pointId, float edgeDistance, float radius);
    SPathPoint* GetPathPoint(unsigned long index);

    SPathPoint m_points[256];  // +0x0000
    int m_pointCount;          // +0x1400
    // Path state initialized by original e6992..e69b4; not read in the current owned range, original fields kept without inventing meanings.
    int m_state1404;
    int m_state1408;
    bool m_flag140c;
    bool m_flag140d;
};

#endif
