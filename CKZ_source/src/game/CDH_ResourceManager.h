// CDH_ResourceManager.h - game resources: image surfaces cached by name, other data from the platform
// resource manager.
#ifndef CDH_RESOURCEMANAGER_H
#define CDH_RESOURCEMANAGER_H

#include <stddef.h>
#include "Vector_ss.h"
#include "CStrChar.h"

namespace com { namespace glu { namespace platform { namespace graphics {
class ICRenderSurface;
} } } }

/** Data of a KEYSET_* resource: a list of resource ids (layout from CDH_SoundManager::GetRandomSound). */  // added by b048
struct CDH_KeySet {  // added by b048
    int m_field00;              // 0x00  // added by b048
    int m_field04;              // 0x04  // added by b048
    unsigned int m_count;       // 0x08  // added by b048
    int m_field0c;              // 0x0c  // added by b048
    const char** m_keys;        // 0x10  // added by b048

    /** Key `index`, NULL when out of range. */  // added by b048
    const char* GetKey(unsigned int index) const  // added by b048
    {  // added by b048
        const char* key = NULL;  // added by b048
        if (m_keys != NULL && m_count > index) {  // added by b048
            key = m_keys[index];  // added by b048
        }  // added by b048
        return key;  // added by b048
    }  // added by b048
};  // added by b048

class CDH_ResourceManager {
public:
    /** A cached image surface (24 bytes; implicit copy constructor). */
    struct TImage {
        com::glu::platform::graphics::ICRenderSurface* m_surface;   // 0x00
        com::glu::platform::components::CStrChar m_name;            // 0x04 resource name
        int m_orientation;                                          // 0x14 InitParam_Orientation (-1: none)
    };

    CDH_ResourceManager();
    virtual ~CDH_ResourceManager();

    /** Delete every cached surface. */
    void flush();
    /** Delete the cached surface `name`; false when it is not cached. */
    bool releaseImage(const char* name);
    /** Load (or take from the cache) the image `name`; `orientation` < 0: as stored. */
    com::glu::platform::graphics::ICRenderSurface* createImageSurface(const char* name, int orientation);
    /** The cached surface `name` (NULL when not cached). */
    com::glu::platform::graphics::ICRenderSurface* getFromCache(const char* name);
    /** Data of resource `name`: an image surface, or the resource data (for a key set a CDH_KeySet).
        `loading` is stored in DGHelper::m_loadingResource. */
    void* getResourceData(const char* name, bool loading);
    /** Release resource `name` (cached image or platform resource). */
    void releaseResource(const char* name);

    Vector<TImage> m_images;    // 0x04
};

#endif
