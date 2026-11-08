// CSceneLighting.cpp - approximate scene lighting for sprites.
#include "CSceneLighting.h"
#include "CSwerve.h"

typedef CSwerveAssertingErrorHandler ErrorHandler;

/** Ambient brightness. */
static const float SCENE_AMBIENT = 0.1f;
/** UpdateSceneLighting runs every UPDATE_PERIOD calls of Update. */
static const int UPDATE_PERIOD = 10;

CSceneLighting::CSceneLighting(swerve::Group<ErrorHandler> scene, swerve::Group<ErrorHandler> lights)
    : m_ambient(SCENE_AMBIENT), m_lightGroup(lights), m_scene(scene), m_updateCounter(0)
{
    UpdateSceneLighting();
}

void CSceneLighting::UpdateSceneLighting()
{
    if (m_lightGroup.m_interface == NULL) {
        return;
    }
    int count = m_lightGroup.getChildCount();
    m_lights.ensureCapacity(count);
    while (m_lights.size() < count) {
        m_lights.addElement(SSceneLight());
    }

    for (int index = 0; index < count; index++) {
        swerve::Transform<ErrorHandler> transform(CSwerve::GetInstance()->m_swerve);
        swerve::Group<ErrorHandler> lightNode = m_lightGroup.getChild(index);
        SSceneLight& light = m_lights[index];
        light.m_intensity = 0.0f;

        float scale[3];
        lightNode.getScale(scale);
        float scaleSum = scale[0] + scale[1] + scale[2];
        lightNode.getTransformTo(m_scene, transform);
        float range = scaleSum / 3.0f;
        float matrix[16];
        transform.get(matrix);
        float x = matrix[3];
        float y = matrix[7];
        float z = matrix[11];

        // The first mesh below the light node gives the brightness (emissive color).
        swerve::Mesh<ErrorHandler> mesh;
        for (int child = 0; child < lightNode.getChildCount(); child++) {
            mesh = swerve::Mesh<ErrorHandler>(lightNode.getChild(child));
            if (mesh.m_interface != NULL) {
                break;
            }
        }
        if (mesh.m_interface == NULL) {
            continue;
        }
        swerve::Appearance<ErrorHandler> appearance;
        for (int submesh = 0; submesh < mesh.getSubmeshCount(); submesh++) {
            appearance = mesh.getAppearance(submesh);
            if (appearance.m_interface != NULL) {
                break;
            }
        }
        if (appearance.m_interface == NULL) {
            continue;
        }
        if (appearance.getMaterial().m_interface != NULL) {
            int color = appearance.getMaterial().getColor(swerve::Material<ErrorHandler>::AMBIENT);
            int brightness = ((color >> 8) & 0xff) + (color & 0xff) + ((color >> 16) & 0xff);
            light.m_position.m_x = x;
            light.m_position.m_y = y;
            light.m_position.m_z = z;
            light.m_intensity = (float)(2.0 * (brightness / 765.0) - 1.0);
            light.m_range = range;
        }
    }
}

CSceneLighting::~CSceneLighting()
{
}

void CSceneLighting::Update()
{
    if (m_updateCounter >= UPDATE_PERIOD) {
        UpdateSceneLighting();
        m_updateCounter = 0;
    }
    m_updateCounter++;
}

float CSceneLighting::GetLuminance(const vec3& position) const
{
    float luminance = m_ambient;
    for (int index = 0; index < m_lights.size(); index++) {
        const SSceneLight& light = m_lights[index];
        float dx = light.m_position.m_x - position.m_x;
        float dy = light.m_position.m_y - position.m_y;
        float dz = light.m_position.m_z - position.m_z;
        float distance = MathLib::Sqrt(dx * dx + dy * dy + dz * dz);
        if (!(distance > light.m_range) && light.m_range > 0.0f) {
            luminance += light.m_intensity * (1.0f - distance / light.m_range);
        }
    }
    if (luminance < 0.0f) {
        return 0.0f;
    }
    // ?: with the constant on the left gives the original vmin.f32
    return (1.0f < luminance) ? 1.0f : luminance;
}
