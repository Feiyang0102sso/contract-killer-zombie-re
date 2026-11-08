// CNGSLockManager.h - shared data locks; a server object that is also a singleton (the CSingleton
// part sits at +0x18, so the singleton table holds this + 0x18). sizeof 0x24.
// (b023x; created as PROVISIONAL by b016x)
// Only the constructor, destructor, removeDeleteAllLocks, suspend and resume survived the
// linker's dead stripping; the lock requests themselves are not known.
#ifndef CNGSLOCKMANAGER_H
#define CNGSLOCKMANAGER_H

#include <stddef.h>
#include "CSingleton.h"
#include "CNGSServerObject.h"
#include "CStrWChar.h"

class CHash_gServe;
class CNGSAttribute;

/** One shared data lock (no vtable; only the fields used by the surviving code are known). */
class CNGSLock {
public:
    /** m_state values seen in suspend() / resume(). */
    enum eLockState {
        LOCK_STATE_0 = 0,
        LOCK_STATE_1 = 1,
        LOCK_STATE_SUSPENDED = 4,       // set by suspend()
        LOCK_STATE_RESUMED = 5          // set by resume()
    };

    unsigned char m_reserved00[0x10];                       // +0x00 not described yet
    int m_state;                                            // +0x10 eLockState
    unsigned char m_reserved14[0x08];                       // +0x14 not described yet
    com::glu::platform::components::CStrWChar m_name;      // +0x1c
    com::glu::platform::components::CStrWChar m_data;      // +0x2c
};

class CNGSLockManager : public CNGSServerObject, public com::glu::platform::components::CSingleton {
public:
    enum { ClassId = 0x4cda0bb7 };

    CNGSLockManager();
    virtual ~CNGSLockManager();

    /** The registered instance, created on first use. */
    static inline CNGSLockManager* GetInstance()
    {
        void* found = NULL;
        CApplet::m_pApp->m_singletons->Find(ClassId, &found);
        CNGSLockManager* instance = static_cast<CNGSLockManager*>((com::glu::platform::components::CSingleton*)found);
        if (instance == NULL) {
            instance = new CNGSLockManager();
        }
        return instance;
    }

    /** Delete every lock and empty the table. */
    void removeDeleteAllLocks();
    /** Locks in state 0, 1 or 5 become suspended (4). */
    void suspend();
    /** Suspended locks become resumed (5). */
    void resume();

    /** (reconstructed, see CNGSLockManager.cpp) ask the server for a shared data collection. */
    unsigned int requestSharedData(int collectionType);
    /** (reconstructed, see CNGSLockManager.cpp) send one shared data attribute. */
    unsigned int saveSharedData(CNGSAttribute* attribute);

    CHash_gServe* m_locks;      // +0x20 lock key -> CNGSLock*
};

#endif
