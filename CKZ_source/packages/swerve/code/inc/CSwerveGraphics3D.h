// CSwerveGraphics3D.h - binds Glu render surfaces as targets of the Swerve renderer.
// (created by b040x, the owner of swerve/code/src/CSwerveGraphics3D.cpp)
#ifndef CSWERVEGRAPHICS3D_H
#define CSWERVEGRAPHICS3D_H

#include <stddef.h>
#include "CClass.h"
#include "CVector.h"
#include "CRectangle.h"
#include "SwerveApi.h"
#include "ICGraphics.h"

namespace com { namespace glu { namespace platform {
namespace graphics { class ICRenderSurface; }
namespace systems { class CEventListener; class CEvent; }
} } }

/**
 * Render state of one off-screen image, kept in malij297_Image::m_glRuntime
 * (created by malij297_OffScreenImageBind). Defined in this header in the original (N_SOL of
 * malij297_glruntime.o); moved here from malij297_glruntime.cpp by b104.
 */
struct GLRuntime {
    com::glu::platform::graphics::ICRenderSurface* m_surface;           // 0x00 target surface of the image
    com::glu::platform::graphics::ICRenderSurface* m_previousTarget;    // 0x04 device target before the image was bound

    GLRuntime()
    {
        m_surface = NULL;
        m_previousTarget = NULL;
    }

    /** Give the device its previous target back. */
    ~GLRuntime()
    {
        com::glu::platform::graphics::ICRenderSurface* previousTarget = m_previousTarget;
        if (previousTarget != NULL) {
            com::glu::platform::graphics::ICGraphics::GetInstance()->SetTargetSurface(*previousTarget);
        }
    }
};

/** Event callback of the renderer (context lost). */
unsigned char CSwerveGraphics3D_EventCB(com::glu::platform::systems::CEvent& event, void* listener);

class CSwerveGraphics3D : public com::glu::platform::core::CClass {
public:
    /** Runtime class id. */
    static const unsigned int ClassId = 0x27ca4026;

    /** A render surface made for mutable images of one size / format (a resource called m_name). */
    struct MutableSurface {
        char m_name[0x80];                                          // 0x00
        com::glu::platform::graphics::ICRenderSurface* m_surface;   // 0x80
    };

    CSwerveGraphics3D();
    virtual ~CSwerveGraphics3D();

    /** Create the renderer and its resources. Returns 1 on success. */
    unsigned char Init();
    /** Release everything Init created. */
    void Free();
    /** A render surface at least as large as `image` (created as a resource when there is none). */
    com::glu::platform::graphics::ICRenderSurface* GetMutableSurface(const malij297_Image* image);
    /** Make `target` the target of the renderer (`viewport` NULL: the whole surface). Returns 0 on success. */
    int BindTarget(com::glu::platform::graphics::ICRenderSurface* target,
                   const com::glu::platform::components::CRectangle<short>* viewport, int flags);
    /** Finish rendering into a mutable image. */
    void BindMutableImageEnd();
    /** Release the current target. Returns the result of the renderer. */
    int ReleaseTarget();

    IGraphics3D* m_graphics3D;                                          // 0x08
    com::glu::platform::graphics::ICRenderSurface* m_target;            // 0x0c
    com::glu::platform::graphics::ICRenderSurface* m_previousTarget;    // 0x10 target of the device before BindTarget
    IObject3D* m_boundImage;                                            // 0x14 mutable image being rendered into
    com::glu::platform::components::CRectangle<short> m_viewport;       // 0x18
    com::glu::platform::graphics::ICRenderSurface* m_dummySurface;      // 0x20
    com::glu::platform::core::CVector<MutableSurface*> m_mutableSurfaces;  // 0x24
    malij297_Environment m_environment;                                 // 0x3c
    com::glu::platform::systems::CEventListener* m_listener;            // 0xd4
};

#endif
