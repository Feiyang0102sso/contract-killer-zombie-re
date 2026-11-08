// CNGSDirectFileDownload.h - downloads files listed by the game (or by batch control files) into the
// application cache, checking their CRC; one request at a time through its CNGSJSONData base (b025x).
// Singleton, sizeof 0xec.
#ifndef CNGSDIRECTFILEDOWNLOAD_H
#define CNGSDIRECTFILEDOWNLOAD_H

#include <stddef.h>
#include "CSingleton.h"
#include "CNGSJSONData.h"
#include "CNGSDirectFileDownloadRequest.h"
#include "CStrWChar.h"
#include "CVector.h"

class CNotificationHandler;

class CNGSDirectFileDownload : public com::glu::platform::components::CSingleton, public CNGSJSONData {
public:
    enum { ClassId = 0x792281fb };

    CNGSDirectFileDownload();
    virtual ~CNGSDirectFileDownload();

    /** The registered instance, created on first use. */
    static inline CNGSDirectFileDownload* GetInstance()
    {
        void* found = NULL;
        CApplet::m_pApp->m_singletons->Find(ClassId, &found);
        CNGSDirectFileDownload* instance = (CNGSDirectFileDownload*)found;
        if (instance == NULL) {
            instance = new CNGSDirectFileDownload();
        }
        return instance;
    }

    /** Cache sub folder "<game id>[/<folder>]" (created now) and the "data/..." URL folder. */
    void SetFolder(com::glu::platform::components::CStrWChar& folder);
    const wchar_t* GetFolder();
    void AddToDownloadQueue(CNGSDirectFileDownloadRequest* request);
    /** Take the first queued request and start (or skip) its download. */
    void StartNextDownload();
    void HandleDataResponse(char* data, int length, bool notModified);
    void HandleEncodedResponse(char* data, int length, bool notModified);
    void HandleBatchResponse(char* data, int length, bool notModified);
    /** Update the listed request or append `request`; 1 when it was already listed. */
    unsigned char AddToCompletedList(CNGSDirectFileDownloadRequest* request, eCNGSDirectFileDownloadDownloadStatus status);
    /** Tell the notification handler about a finished request. */
    void DownloadComplete(CNGSDirectFileDownloadRequest* request);
    com::glu::platform::core::CVector<CNGSDirectFileDownloadRequest*> GetQueued();
    /** Set the status of `request` and of the completed request with the same file name; 1 when listed. */
    unsigned char UpdateCompletedList(CNGSDirectFileDownloadRequest* request, eCNGSDirectFileDownloadDownloadStatus status);
    /** Count one more file of the batch of `request`; the batch size once complete, 0 before, -1 without batch. */
    int IncrementDownloadCount(const CNGSDirectFileDownloadRequest* request);
    void HandleUpdate(int elapsedMs);
    /** Base64 string `name` at `path` decoded into a malloc'd block (*data); its size or -1. */
    int GetDecodedBinaryData(com::glu::platform::components::CStrWChar path, com::glu::platform::components::CStrWChar name,
                             void** data);
    void registerNotificationHandler(CNotificationHandler* handler);
    /** Queue every "payload/<n>" entry of the downloaded batch control file. */
    unsigned char processControlFile();

    com::glu::platform::components::CStrWChar m_folder;                         // +0x90 cache sub folder
    CNGSDirectFileDownloadRequest* m_currentRequest;                            // +0xa0
    com::glu::platform::components::CStrWChar m_dataFolder;                     // +0xa4 "data/<folder>"
    com::glu::platform::core::CVector<CNGSDirectFileDownloadRequest*> m_queue;      // +0xb4 owned
    com::glu::platform::core::CVector<CNGSDirectFileDownloadRequest*> m_completed;  // +0xcc owned
    bool m_downloading;                                                         // +0xe4 the queue is processed
    CNotificationHandler* m_notificationHandler;                                // +0xe8
};

/** Response callback calling a CNGSDirectFileDownload member. */
class CNGSDirectFileDownloadRequestFunctor : public CNGSDataRequestFunctor {
public:
    typedef void (CNGSDirectFileDownload::*Callback)(char* data, int length, bool notModified);

    CNGSDirectFileDownloadRequestFunctor(CNGSDirectFileDownload* target, Callback callback)
        : m_target(target)
        , m_callback(callback)
    {
    }

    virtual ~CNGSDirectFileDownloadRequestFunctor() {}

    /** Unused (downloads deliver raw data). */
    virtual void requestCompleteCallback(CObjectMap* response);
    virtual void requestCompleteCallback(char* data, int length, bool notModified);

    CNGSDirectFileDownload* m_target;   // +0x08
    Callback m_callback;                // +0x0c
};

#endif
