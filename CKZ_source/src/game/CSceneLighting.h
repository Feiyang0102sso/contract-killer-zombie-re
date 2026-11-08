// CSceneLighting.h - approximate scene lighting for sprites: point lights taken from the light group of
// the scene (position, emissive brightness, size), evaluated with GetLuminance.
#ifndef CSCENELIGHTING_H
#define CSCENELIGHTING_H

#include "tws.h"
#include "MathLib.h"
#include "Vector_ss.h"

/** Scene lighting (0x20 bytes). */
class CSceneLighting {
public:
    /** One light (20 bytes). */
    struct SSceneLight {
        SSceneLight()
        {
            m_intensity = 0.0f;
            m_range = 1.0f;
        }

        vec3 m_position;        // 0x00 in scene coordinates
        float m_intensity;      // 0x0c -1..1 (from the emissive color of the light mesh)
        float m_range;          // 0x10 mean scale of the light node
    };

    /** `scene`: the scene root; `lights`: the group whose children are the lights. */
    CSceneLighting(swerve::Group<CSwerveAssertingErrorHandler> scene,
                   swerve::Group<CSwerveAssertingErrorHandler> lights);
    ~CSceneLighting();

    /** Read the lights again. */
    void UpdateSceneLighting();
    /** Read the lights again every 10 calls. */
    void Update();
    /** Brightness at `position` (0..1). */
    float GetLuminance(const vec3& position) const;

    Vector<SSceneLight> m_lights;                               // 0x00
    float m_ambient;                                            // 0x10
    swerve::Group<CSwerveAssertingErrorHandler> m_lightGroup;   // 0x14
    swerve::Group<CSwerveAssertingErrorHandler> m_scene;        // 0x18
    int m_updateCounter;                                        // 0x1c
};

#endif
