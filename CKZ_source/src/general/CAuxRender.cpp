// CAuxRender.cpp - debug drawing of spheres, boxes and lines in the 3D scene (queued as tasks).
#include "CAuxRender.h"
#include <math.h>
#include "CSwerve.h"

/** Runtime object kind created for the debug renderer. */
static const int SWERVE_OBJECT_KIND = 15;
// The original angle conversion widens this single-precision constant to double.
static const float PI = 3.14159f;
/** Length of the replacement line drawn when the direction cannot be used. */
static const float FALLBACK_LINE_LENGTH = 10.0f;

inline SwerveCreatedObject::SwerveCreatedObject(int kind)
{
    IQueryInterface* object;
    CSwerve::GetInstance()->m_swerve->m_functions->CreateObject(kind, (void**)&object);
    m_interface = object;
}

CAuxRender::CAuxRender()
    : m_object(SWERVE_OBJECT_KIND)
{
    Cleanup();
    m_tasks.reserve(MAX_TASKS);
}

void CAuxRender::Cleanup()
{
    m_root.Clear();
    for (int index = 0; index < 4; index++) {
        m_shapes[index].Clear();
    }
}

CAuxRender::~CAuxRender()
{
    Cleanup();
}

void CAuxRender::DrawSphere(const vec3& center, float radius, int color)
{
    if (m_tasks.m_size >= m_tasks.m_capacity) {
        return;
    }
    // Read the input before writing the queued task; it may reference the task buffer.
    vec3 position = center;
    STask& task = m_tasks.m_data[m_tasks.m_size];
    task.m_type = TASK_SPHERE;
    task.m_position = position;
    task.m_size = vec3(radius, radius, radius);
    task.m_axis = vec3(1.0f, 0.0f, 0.0f);
    task.m_angle = 0.0f;
    task.m_color = color;
    m_tasks.m_size++;
}

void CAuxRender::DrawBox(const vec3& minimum, const vec3& maximum, int color)
{
    if (m_tasks.m_size >= m_tasks.m_capacity) {
        return;
    }
    // The original reads both corners before the first task store.
    vec3 size = maximum;
    vec3 position = minimum;
    STask& task = m_tasks.m_data[m_tasks.m_size];
    task.m_type = TASK_BOX;
    task.m_position = position;
    task.m_size = size;
    task.m_axis = vec3(1.0f, 0.0f, 0.0f);
    task.m_angle = 0.0f;
    task.m_color = color;
    m_tasks.m_size++;
}

void CAuxRender::DrawLine(const vec3& start, const vec3& end, float width, int color)
{
    vec3 direction = end - start;
    float lengthSqr = direction.LengthSqr();
    float length = MathLib::InvSqrt(lengthSqr) * lengthSqr;
    vec3 normal = direction * MathLib::InvSqrt(lengthSqr);
    // the line mesh points along +z: rotate it onto the direction
    vec3 up(0.0f, 0.0f, 1.0f);
    float cosine = normal.Dot(up);
    if (!(cosine <= 1.0f && cosine >= -1.0f)) {
        DrawLine(start, start + up * FALLBACK_LINE_LENGTH, width, color);
        return;
    }
    float angle = (float)(acos(cosine) * 180.0 / PI);
    vec3 axis = up.Cross(normal);
    if (axis.m_x == 0.0f && axis.m_y == 0.0f && axis.m_z == 0.0f) {
        axis = vec3(1.0f, 0.0f, 0.0f);
        angle = 0.0f;
    }
    if (m_tasks.m_size >= m_tasks.m_capacity) {
        return;
    }
    // Read the start coordinates before any writes to the task buffer.
    float startX = start.m_x;
    float startY = start.m_y;
    float startZ = start.m_z;
    STask& task = m_tasks.m_data[m_tasks.m_size];
    task.m_type = TASK_LINE;
    task.m_position.m_x = startX;
    task.m_position.m_y = startY;
    task.m_position.m_z = startZ;
    task.m_size = vec3(width, width, length);
    task.m_axis.m_x = axis.m_x;
    task.m_axis.m_y = axis.m_y;
    task.m_color = color;
    task.m_axis.m_z = axis.m_z;
    task.m_angle = angle;
    m_tasks.m_size++;
}
