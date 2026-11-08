// CSwerve.h - application singleton owning the Swerve (M3G) runtime and its renderer.
// (created by b040x, the owner of swerve/code/src/CSwerve.cpp)
#ifndef CSWERVE_H
#define CSWERVE_H

#include <stddef.h>
#include "CSingleton.h"
#include "CStrCharBuffer.h"
#include "SwerveApi.h"

class CSwerveGraphics3D;

class CSwerve : public com::glu::platform::components::CSingleton {
public:
    /** Key in the application singleton table. */
    enum { SingletonId = 0x36412505 };

    CSwerve();
    virtual ~CSwerve();

    /** The application's instance, created on first use. */
    static inline CSwerve* GetInstance()
    {
        if (m_pSwerve == NULL) {
            CSwerve* found = NULL;
            CApplet::m_pApp->m_singletons->Find(SingletonId, (void**)&found);
            CSwerve* swerve = found;
            if (swerve == NULL) {
                swerve = new CSwerve();
            }
            m_pSwerve = swerve;
        }
        return m_pSwerve;
    }

    /** Create the runtime and the renderer (OpenGL ES 1.x only). Returns 1 on success. */
    unsigned char Init();
    /** Destroy the renderer and the runtime. */
    void Free();
    /** Log `error` of the runtime together with its error text; `context` names the caller. */
    static void LogError(int error, const char* context);
    /** Load the M3G resource `name` and return its first root object (NULL on failure). */
    IObject3D* LoadM3GResource(const char* name);
    /** Hash of `text` used as M3G user id (30 bits). */
    static int GenerateUserID(const char* text);

    static CSwerve* m_pSwerve;      // the instance (also in the singleton table)

    ISwerve* m_swerve;                                  // 0x08 runtime root object
    CSwerveGraphics3D* m_graphics3D;                    // 0x0c
    com::glu::platform::components::CStrCharBuffer m_buffer;  // 0x10 (not used by this module)
};

#endif
