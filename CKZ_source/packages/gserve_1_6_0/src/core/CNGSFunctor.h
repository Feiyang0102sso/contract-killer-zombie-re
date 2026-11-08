// CNGSFunctor.h - completion callbacks of gServe server objects (created by b016x, gserve core).
// A CNGSNotifyFunctor is handed to SyncAllFromServer()/SyncAllToServer(); the server object calls
// notify() once all outstanding requests finished and then deletes it.
#ifndef CNGSFUNCTOR_H
#define CNGSFUNCTOR_H

class CNGSNotifyFunctor {
public:
    CNGSNotifyFunctor() {}
    virtual ~CNGSNotifyFunctor() {}

    virtual void notify() = 0;
};

// ---- added by b025x: ProfileManagerFunctor (the original keeps it in this header: CProfileManager.o
// takes its inline code from CNGSFunctor.h) ----
#include "CNGSServerRequest.h"  // added by b025x
#include "CVector.h"  // added by b025x

class CProfileManager;  // added by b025x

/** Reply callback of CProfileManager requests; remembers the collections of the request. */  // added by b025x
class ProfileManagerFunctor : public CNGSServerRequestFunctor {  // added by b025x
public:  // added by b025x
    typedef void (CProfileManager::*Callback)(CObjectMap* response, ProfileManagerFunctor* functor);  // added by b025x

    ProfileManagerFunctor(CProfileManager* target, Callback callback,  // added by b025x
                          const com::glu::platform::core::CVector<int>& collectionTypes)  // added by b025x
        : m_target(target)  // added by b025x
        , m_callback(callback)  // added by b025x
    {  // added by b025x
        for (int index = 0; index < collectionTypes.Size(); index++) {  // added by b025x
            m_collectionTypes.Add(collectionTypes[index]);  // added by b025x
        }  // added by b025x
    }  // added by b025x

    virtual ~ProfileManagerFunctor() {}  // added by b025x

    virtual void requestCompleteCallback(CObjectMap* response)  // added by b025x
    {  // added by b025x
        if (m_callback) {  // added by b025x
            (m_target->*m_callback)(response, this);  // added by b025x
        }  // added by b025x
    }  // added by b025x

    CProfileManager* m_target;                                          // +0x08  added by b025x
    Callback m_callback;                                                // +0x0c  added by b025x
    com::glu::platform::core::CVector<int> m_collectionTypes;           // +0x14  added by b025x
};  // added by b025x

#endif
