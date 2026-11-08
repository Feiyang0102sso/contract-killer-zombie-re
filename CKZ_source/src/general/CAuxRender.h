// CAuxRender.h - debug drawing of spheres, boxes and lines in the 3D scene (queued as tasks).
#ifndef CAUXRENDER_H
#define CAUXRENDER_H

#include "MathLib.h"
#include "CFixedVector.h"
#include "tws.h"

/** Reference to a runtime object that the Swerve runtime creates by kind (CreateObject). */
class SwerveCreatedObject {
public:
    explicit SwerveCreatedObject(int kind);

    ~SwerveCreatedObject()
    {
        if (m_interface != NULL) {
            m_interface->Release();
        }
    }

    IQueryInterface* m_interface;   // 0x00 owned reference
};

class CAuxRender {
public:
    /** Kinds of debug shapes. */
    enum ETaskType {
        TASK_BOX = 0,
        TASK_SPHERE = 1,
        TASK_LINE = 3
    };

    /** One queued shape. */
    struct STask {
        STask()
            : m_axis(1.0f, 0.0f, 0.0f)
        {
            m_angle = 0.0f;
        }

        int m_type;         // 0x00 ETaskType
        vec3 m_position;    // 0x04 sphere center / box minimum / line start
        vec3 m_size;        // 0x10 radius x3 / box maximum / (width, width, length)
        vec3 m_axis;        // 0x1c rotation axis
        float m_angle;      // 0x28 rotation in degrees
        int m_color;        // 0x2c
    };

    /** Most shapes queued at a time. */
    enum { MAX_TASKS = 0x1080 };

    CAuxRender();
    virtual ~CAuxRender();

    /** Release the shape objects. */
    void Cleanup();
    void DrawSphere(const vec3& center, float radius, int color);
    void DrawBox(const vec3& minimum, const vec3& maximum, int color);
    void DrawLine(const vec3& start, const vec3& end, float width, int color);

    swerve::QueryInterface<CSwerveAssertingErrorHandler> m_root;        // 0x04
    swerve::QueryInterface<CSwerveAssertingErrorHandler> m_shapes[4];   // 0x08
    SwerveCreatedObject m_object;                                   // 0x18 runtime object of kind 15
    CFixedVector<STask> m_tasks;                                    // 0x1c
};

#endif
