// CNGSContentManager.h - uploads / downloads content files (profile data blobs) of the gServe
// user (b025x; replaces the b016x PROVISIONAL). Singleton, sizeof 0x50.
#ifndef CNGSCONTENTMANAGER_H
#define CNGSCONTENTMANAGER_H

#include <stddef.h>
#include "CSingleton.h"
#include "CNGSServerObject.h"
#include "CStrWChar.h"

namespace com { namespace glu { namespace platform { namespace network {
class CHttpTransport;
} } } }

class CObjectMap;
class CObjectMapObject;
class CNGSServerRequest;
class CNGSContentRequestFunctor;
class CSaveRestoreInterface;

/** getContentManagerStatus() results. */
enum eContentManagerStatus {
    CONTENT_MANAGER_UPLOADING = 0,
    CONTENT_MANAGER_DOWNLOADING = 1,
    CONTENT_MANAGER_BUSY = 2,           // a read request is outstanding
    CONTENT_MANAGER_READY = 3,
    CONTENT_MANAGER_LOCKED = 4          // CProfileManager is loading / saving
};

class CNGSContentManager : public com::glu::platform::components::CSingleton, public CNGSServerObject {
public:
    enum { ClassId = 0x2c21f561 };

    CNGSContentManager();
    virtual ~CNGSContentManager();

    /** The registered instance, created on first use. */
    static inline CNGSContentManager* GetInstance()
    {
        void* found = NULL;
        CApplet::m_pApp->m_singletons->Find(ClassId, &found);
        CNGSContentManager* instance = (CNGSContentManager*)found;
        if (instance == NULL) {
            instance = new CNGSContentManager();
        }
        return instance;
    }

    void Clear();
    eContentManagerStatus getContentManagerStatus();
    /** Pump the download transport and deliver a finished download. */
    void HandleUpdate(int elapsedMs);
    void CNGSHandleContentDownloadResponse(bool success, const unsigned char* data, int size);
    /** {"gameid", "filename", "data" (base64)} of an upload. */
    CObjectMapObject* createContentUploadMessageObject(const char* fileName, const unsigned char* data, int size,
                                                       CNGSContentRequestFunctor* functor);
    /** "GLUID<userId>_<fileName>". */
    com::glu::platform::components::CStrWChar generateFilename(int userId, const char* fileName);
    /** Upload `data` as `fileName`; returns the message id (0 when busy). */
    unsigned int UploadContent(const char* fileName, const unsigned char* data, int size);
    void CNGSHandleContentUploadResponse(CObjectMap* response, CNGSContentRequestFunctor* functor);
    /** Upload a CProfileManager collection; the result goes back to the profile manager. */
    void UploadProfileContent(const char* fileName, const unsigned char* data, int size, CSaveRestoreInterface* profileData);
    void Lock();
    void Unlock();

    int m_field20;                                                  // +0x20 cleared only (meaning unknown)
    int m_field24;                                                  // +0x24 cleared only (meaning unknown)
    com::glu::platform::components::CStrWChar m_fileName;           // +0x28 file being uploaded / downloaded
    com::glu::platform::network::CHttpTransport* m_transport;       // +0x38 download in progress
    unsigned char* m_downloadBuffer;                                // +0x3c
    int m_targetUserId;                                             // +0x40 owner of the downloaded file
    CSaveRestoreInterface* m_profileData;                           // +0x44 collection of a profile upload / download
    CNGSServerRequest* m_pendingRequest;                            // +0x48 deleted when a download succeeds
    unsigned char m_isProfileContent;                               // +0x4c result goes to CProfileManager
    unsigned char m_isFriendContent;                                // +0x4d download of a friend's file
    unsigned char m_locked;                                         // +0x4e
};

#endif
