// ICResourceManager.h - resource manager interface: creates, finds, reference-counts and
// destroys CResource objects by handle or name. One instance per application (created by the
// game's CAppFactory). (created by b011x; the iterator interface was drafted by b000x)
#ifndef ICRESOURCEMANAGER_H
#define ICRESOURCEMANAGER_H

#include <stddef.h>
#include <wchar.h>
#include "CApplet.h"
#include "CApp.h"
#include "CHash.h"

namespace com { namespace glu { namespace platform {

namespace components {
class CInputStream;
class CStrWChar;
}

namespace systems {

class CResource;

class ICResourceManager {
public:
    /** Key in the application singleton table: CStringToKey("ICResourceManager"). */
    enum { SingletonId = 0x7f5df23a };

    /** Default Initialize() arguments: hash buckets and initially allocated hash nodes. */
    static const unsigned long DefaultTableSize;
    static const unsigned long DefaultNumOfElementsInInitialAlloc;

    /** Walks resources whose ids follow each other in the archive name table. */
    class CConsecutiveResourceIdItr {
    public:
        /** Seek() origin; only a non-zero origin moves (meaning of the values inferred). */
        enum SeekFrom {
            SeekFrom_Start = 0,
            SeekFrom_Current = 1
        };

        /** Handle (name key) of the current resource. */
        virtual unsigned int GetHandle() = 0;
        /** Name of the current resource, NULL past the end. */
        virtual const char* GetName() = 0;
        virtual CConsecutiveResourceIdItr& Next() = 0;
        virtual CConsecutiveResourceIdItr& Prev() = 0;
        virtual CConsecutiveResourceIdItr& Seek(SeekFrom from, int count) = 0;
        virtual CConsecutiveResourceIdItr& operator++() = 0;
        virtual CConsecutiveResourceIdItr& operator++(int) = 0;
        virtual CConsecutiveResourceIdItr& operator--() = 0;
        virtual CConsecutiveResourceIdItr& operator--(int) = 0;
    };

    /** CreateResources() list entries; a list ends with a zero handle / NULL name. */
    struct CreationInfoHnd {
        unsigned int m_mimeType;                // 0x00
        components::CInputStream* m_stream;     // 0x04
        unsigned int m_handle;                  // 0x08
    };

    struct CreationInfoStr {
        unsigned int m_mimeType;                // 0x00
        components::CInputStream* m_stream;     // 0x04
        const char* m_name;                     // 0x08
    };

    struct CreationInfoWStr {
        unsigned int m_mimeType;                // 0x00
        components::CInputStream* m_stream;     // 0x04
        const wchar_t* m_name;                  // 0x08
    };

    /** The game's resource manager (CAppFactory::CreateResourceManager). */
    static ICResourceManager* CreateInstance();

    /** The application's resource manager: cached in CApp, created on first use. */
    static inline ICResourceManager* GetInstance()
    {
        framework::CApp* app = CApplet::m_pApp;
        ICResourceManager* instance = NULL;
        if (app != NULL) {
            if (app->m_resourceManager == NULL) {
                ICResourceManager* found = NULL;
                app->m_singletons->Find(SingletonId, (void**)&found);
                ICResourceManager* created = found;
                if (created == NULL) {
                    created = CreateInstance();
                }
                app->m_resourceManager = created;
            }
            instance = (ICResourceManager*)app->m_resourceManager;
        }
        return instance;
    }

    virtual ~ICResourceManager();

    virtual unsigned char Initialize(unsigned long tableSize, unsigned long initialAllocCount,
                                     unsigned char growable) = 0;
    virtual void Destroy() = 0;
    virtual CConsecutiveResourceIdItr* CreateAndInitConsecutiveResourceIdItr(unsigned int handle,
                                                                             unsigned int count) = 0;
    virtual CConsecutiveResourceIdItr* CreateAndInitConsecutiveResourceIdItr(const char* name,
                                                                             unsigned int count) = 0;
    virtual CConsecutiveResourceIdItr* CreateAndInitConsecutiveResourceIdItr(const wchar_t* name,
                                                                             unsigned int count) = 0;
    virtual unsigned char AddResource(CResource& resource) = 0;
    virtual unsigned char RemoveResource(unsigned int handle) = 0;
    virtual unsigned char RemoveResource(const char* name) = 0;
    virtual unsigned char RemoveResource(const wchar_t* name) = 0;
    virtual unsigned char RemoveResource(CResource& resource) = 0;
    virtual unsigned int CreateResource(unsigned int handle, CResource** resource) = 0;
    virtual unsigned char CreateResources(const unsigned int* handles) = 0;
    virtual unsigned int CreateResource(const CreationInfoHnd& info, CResource** resource) = 0;
    virtual unsigned char CreateResources(const CreationInfoHnd* infos) = 0;
    virtual unsigned int CreateResource(const char* name, CResource** resource) = 0;
    virtual unsigned char CreateResources(const char** names) = 0;
    virtual unsigned int CreateResource(const CreationInfoStr& info, CResource** resource) = 0;
    virtual unsigned char CreateResources(const CreationInfoStr* infos) = 0;
    virtual unsigned int CreateResource(const wchar_t* name, CResource** resource) = 0;
    virtual unsigned char CreateResources(const wchar_t** names) = 0;
    virtual unsigned int CreateResource(const CreationInfoWStr& info, CResource** resource) = 0;
    virtual unsigned char CreateResources(const CreationInfoWStr* infos) = 0;
    /** Find a created resource; *resource is NULL when there is none. */
    virtual unsigned char GetResource(unsigned int handle, CResource** resource) = 0;
    virtual unsigned char GetResource(const char* name, CResource** resource) = 0;
    virtual unsigned char GetResource(const wchar_t* name, CResource** resource) = 0;
    virtual unsigned char ReleaseResource(unsigned int handle) = 0;
    virtual void ReleaseResources(const unsigned int* handles) = 0;
    virtual unsigned char ReleaseResource(const char* name) = 0;
    virtual void ReleaseResources(const char** names) = 0;
    virtual unsigned char ReleaseResource(const wchar_t* name) = 0;
    virtual void ReleaseResources(const wchar_t** names) = 0;
    /** Drop one reference; the resource is deleted and *resource cleared at zero. */
    virtual unsigned char ReleaseResource(CResource** resource) = 0;
    virtual unsigned char DestroyResource(unsigned int handle) = 0;
    virtual void DestroyResources(const unsigned int* handles) = 0;
    virtual unsigned char DestroyResource(const char* name) = 0;
    virtual void DestroyResources(const char** names) = 0;
    virtual unsigned char DestroyResource(const wchar_t* name) = 0;
    virtual void DestroyResources(const wchar_t** names) = 0;
    virtual unsigned char DestroyResources() = 0;
    virtual void SubstituteResourceInfoInPlaceOfStringData(unsigned char substitute) = 0;
    virtual const components::CStrWChar& GetVersion() const = 0;
    virtual void GetVersion(unsigned int& major, unsigned int& minor) const = 0;
    /** Create (or reference) the resource `handle` / `name` of `mimeType`, loading it from `stream`. */
    // b118: this internal overload belongs to the derived resource manager; the original interface vtable ends after the two GetVersion overloads.
};

} } } }

#endif
