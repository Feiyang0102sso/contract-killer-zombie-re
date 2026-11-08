// PROVISIONAL (created by b057x): owner src/bounty_hunter/Manager/CUpdateManager.cpp not written yet
// CUpdateManager.h - downloadable content updates (a gServe notification handler); only what
// src/general/App.cpp uses is declared. Slots after the CNotificationHandler destructor (+0xe4).
// Restored by b079: original virtual names and three path strings, total size 0x90.
#ifndef CUPDATEMANAGER_H
#define CUPDATEMANAGER_H

#include "CNotificationHandler.h"

class CUpdateManager : public CNotificationHandler {
public:
    /** Called when downloaded updates were installed. */
    typedef void (*InstalledCallback)();

    CUpdateManager();
    virtual ~CUpdateManager();

    virtual void init();                        // 0xe8 (former provisional Init)
    virtual void checkUpdates();                // 0xec (former provisional Start)
    virtual int hasCompleteUpdates() const;     // 0xf0 (former provisional GetState)
    virtual int installUpdates(bool silent);    // 0xf4 (former provisional SetState(int state))
    virtual bool hasNewUpdatesInstalled() const { return m_newUpdatesInstalled; } // former VirtualSlotF8
    virtual void update(unsigned int elapsed);  // 0xfc (former provisional Update(unsigned long elapsed))
    virtual void fileComplete(const CNGSDirectFileDownloadRequest& request);
    virtual void batchComplete();
    virtual void makeWorkPath(com::glu::platform::components::CStrWChar& path) const;
    virtual void makeUpdatesListPath(com::glu::platform::components::CStrWChar& path) const;
    virtual void makeCompleteUpdatesListPath(com::glu::platform::components::CStrWChar& path) const;

    virtual unsigned char HandleFileDownloadedMessage(unsigned char success, CNGSDirectFileDownloadRequest& request);
    virtual unsigned char HandleBatchFileDownloadStartedMessage(const com::glu::platform::components::CStrWChar& batch, int count);
    virtual unsigned char HandleBatchFileDownloadCompleteMessage(const com::glu::platform::components::CStrWChar& batch, int count);

    // Compatibility names for previous callers; these forward to the recovered virtual slots.
    inline void Init() { init(); }
    inline void Start() { checkUpdates(); }
    inline int GetState() { return hasCompleteUpdates(); }
    inline void SetState(int state) { installUpdates(state != 0); }
    inline void Update(unsigned long elapsed) { update(elapsed); }

    void copyFromCacheToWork(const wchar_t* fileName, const wchar_t* folder) const;
    static int copyFile(const wchar_t* source, const wchar_t* destination);

    /** Full path of the downloaded copy of resource file `fileName`. */  // added by b055x
    void makeResourcePath(const wchar_t* fileName, com::glu::platform::components::CStrWChar& path) const;  // added by b055x

    InstalledCallback m_onInstalled;            // 0x58
    // Former m_data5c[0x90 - 0x5c]: layout not known yet.
    com::glu::platform::components::CStrWChar m_workPath;       // 0x5c
    com::glu::platform::components::CStrWChar m_updatesPath;    // 0x6c
    com::glu::platform::components::CStrWChar m_completePath;   // 0x7c
    bool m_downloadFailed;                                     // 0x8c
    bool m_newUpdatesInstalled;                                // 0x8d
};

#endif
