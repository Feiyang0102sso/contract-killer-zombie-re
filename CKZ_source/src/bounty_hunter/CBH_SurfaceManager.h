// PROVISIONAL (created by b057x): owner src/bounty_hunter/CBH_SurfaceManager.cpp not written yet
// CBH_SurfaceManager.h - shared render surfaces loaded by name; only what src/general uses.
// b074: complete owner recovered. Historical provisional notes retained below.
#ifndef CBH_SURFACEMANAGER_H
#define CBH_SURFACEMANAGER_H

#include "HashTable.h"
#include "xString.h"

namespace com { namespace glu { namespace platform { namespace graphics {
class ICRenderSurface;
} } } }

class CBH_SurfaceManager {
public:
    /** A loaded surface and its users (12 bytes: HashTable::Add copies three words). */  // changed by b077: defined here
    struct CachedSurface {  // changed by b077 (the HashTable bodies in HashTable.h need the complete type)
        com::glu::platform::graphics::ICRenderSurface* m_surface;   // 0x00 (ReleaseSurface(surface) compares it)  // added by b077
        // Former int m_field04: 0x04 not known yet (added by b077).
        const char* m_name; // borrowed original name; 0x11963c stores the caller's pointer
        // Former int m_field08: 0x08 not known yet (added by b077).
        unsigned short m_references; // 0x08 ldrh/strh, 16-bit wrap is intentional
    };  // added by b077

    /** Load (or share) the surface of resource `name`. */
    com::glu::platform::graphics::ICRenderSurface* CreateSurface(const char* name, bool keep, bool mipmaps);
    /** The loaded surface of resource `name` (NULL when not loaded). */  // added by b055x
    com::glu::platform::graphics::ICRenderSurface* GetFromCache(const char* name);  // added by b055x
    /** Drop one user of the surface `name`. */
    bool ReleaseSurface(const char* name);
    /** Drop one user of `surface`. */  // added by b077
    bool ReleaseSurface(com::glu::platform::graphics::ICRenderSurface* surface);  // added by b077
    /** Release every surface. */
    void Clear();
    bool ImageInCache(const char* name);
    unsigned char IsImageExists(const char* name);
    unsigned int GetSurfaceWidth(com::glu::platform::graphics::ICRenderSurface* surface);
    unsigned int GetSurfaceHeight(com::glu::platform::graphics::ICRenderSurface* surface);

    HashTable<XString, CachedSurface> m_surfaces;   // 0x00 by resource name
};

#endif
