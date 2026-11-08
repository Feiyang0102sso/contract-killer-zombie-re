// CSwerveLoader.h - loads M3G files (and plain images) into Swerve objects, resolving external
// references (XREFs) through the Glu file system and resource manager.
// (created by b040x, the owner of swerve/code/src/CSwerveLoader.cpp)
#ifndef CSWERVELOADER_H
#define CSWERVELOADER_H

#include <stddef.h>
#include "CArrayInputStream.h"
#include "CStrWChar.h"
#include "CVector.h"
#include "CApplet.h"
#include "CApp.h"
#include "ICResourceManager.h"
#include "CResource.h"
#include "SwerveApi.h"

/** Input stream over the data of a resource; the resource is released when the stream closes. */
class CResourceInputStream : public com::glu::platform::components::CArrayInputStream {
public:
    CResourceInputStream()
    {
    }

    virtual ~CResourceInputStream()
    {
    }

    /** Open the resource called `name`. Returns 1 on success. */
    unsigned char Open(const wchar_t* name)
    {
        unsigned char opened = 0;
        com::glu::platform::systems::ICResourceManager* resourceManager =
            (com::glu::platform::systems::ICResourceManager*)CApplet::m_pApp->m_resourceManager;
        com::glu::platform::systems::CResource* resource = NULL;
        resourceManager->CreateResource(name, &resource);
        if (resource != NULL) {
            com::glu::platform::core::CVector<unsigned char>* data =
                (com::glu::platform::core::CVector<unsigned char>*)resource->GetData();
            if (CArrayInputStream::Open(data->m_data, data->m_size)) {
                m_name = name;
                m_closeContext = (void*)m_name.m_string;
                m_onClose = OnClose;
                opened = 1;
            }
        }
        return opened;
    }

    /** Close notification: release the resource (`context` is its name). */
    static unsigned char OnClose(void* context, com::glu::platform::components::CInputStream& stream)
    {
        com::glu::platform::systems::ICResourceManager* resourceManager =
            (com::glu::platform::systems::ICResourceManager*)CApplet::m_pApp->m_resourceManager;
        return resourceManager->ReleaseResource((const wchar_t*)context);
    }

    com::glu::platform::components::CStrWChar m_name;  // 0x20 resource name
};

/** Opens `uri` as a file (application or data directory) or as a resource named after it. */
com::glu::platform::components::CInputStream* CSwerveLoader_getResourceAsStream(const wchar_t* uri);

class CSwerveLoader {
public:
    /** Load `size` bytes of `data` from `offset`; the root objects are appended to `objects`. Returns 0 on success. */
    static int load(unsigned int size, const unsigned char* data, int offset,
                    com::glu::platform::core::CVector<IObject3D*>& objects);
    /**
     * Load an M3G file or an image from `stream`. `references` resolve XREFs by user id, `uri`
     * names the stream (for relative XREFs and messages), `imageFormat` / `xrefType` come from the
     * referencing XREF (-1 / 0 at top level).
     */
    static int loadStreamer(unsigned int referenceCount, IObject3D** references, int imageFormat,
                            com::glu::platform::components::CInputStream* stream, const wchar_t* uri,
                            int xrefType, com::glu::platform::core::CVector<IObject3D*>& objects);
    /** Resolve the pending XREFs of m_loader. Returns 0 on success. */
    int processXREFs(unsigned int referenceCount, IObject3D** references, const wchar_t* baseUri);
    /** True unless `uri` has a scheme or is absolute. */
    static bool isRelative(const com::glu::platform::components::CStrWChar& uri);
    /** `relative` resolved against the directory of `base`. */
    static com::glu::platform::components::CStrWChar resolveURI(const com::glu::platform::components::CStrWChar& base,
                                                               const com::glu::platform::components::CStrWChar& relative);

    ILoader* m_loader;      // 0x00
};

#endif
