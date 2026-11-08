// CNGSLockManager.cpp - shared data locks of gServe.
// The linker dead-stripped most of this object: only the constructor, destructor,
// removeDeleteAllLocks, suspend and resume survived, plus the weak CVector<int>,
// CVector<CNGSAttribute*> and CAttributeManager functions that the stripped lock requests used
// (this object is the first in link order to emit them). requestSharedData / saveSharedData are
// minimal reconstructions of those requests: their names and exact shape are not known.
#include "CNGSLockManager.h"
#include "CHash_gServe.h"
#include "CNGSAttributeManager.h"
#include "CVector.h"

using com::glu::platform::components::CSingleton;
using com::glu::platform::core::CVector;

CNGSLockManager::CNGSLockManager()
    : CSingleton(ClassId)
{
    CHash_gServe* locks = new CHash_gServe();
    locks->Init(100, 20, 1);
    m_locks = locks;
}

CNGSLockManager::~CNGSLockManager()
{
    removeDeleteAllLocks();
    if (m_locks != NULL) {
        delete m_locks;
        m_locks = NULL;
    }
    m_locks = NULL;
}

void CNGSLockManager::removeDeleteAllLocks()
{
    // only the first lock of every bucket chain is deleted (as in the original)
    for (int bucket = 0; bucket < m_locks->m_bucketCount; bucket++) {
        CHash_gServe::Entry* entry = m_locks->m_buckets[bucket];
        if (entry != NULL && entry->m_value != NULL) {
            delete (CNGSLock*)entry->m_value;
        }
    }
    m_locks->RemoveAll();
}

void CNGSLockManager::suspend()
{
    for (int bucket = 0; bucket < m_locks->m_bucketCount; bucket++) {
        CHash_gServe::Entry* entry = m_locks->m_buckets[bucket];
        if (entry != NULL) {
            CNGSLock* lock = (CNGSLock*)entry->m_value;
            switch (lock->m_state) {
            case CNGSLock::LOCK_STATE_0:
            case CNGSLock::LOCK_STATE_1:
            case CNGSLock::LOCK_STATE_RESUMED:
                lock->m_state = CNGSLock::LOCK_STATE_SUSPENDED;
                break;
            default:
                break;
            }
        }
    }
}

void CNGSLockManager::resume()
{
    for (int bucket = 0; bucket < m_locks->m_bucketCount; bucket++) {
        CHash_gServe::Entry* entry = m_locks->m_buckets[bucket];
        if (entry != NULL) {
            CNGSLock* lock = (CNGSLock*)entry->m_value;
            if (lock->m_state == CNGSLock::LOCK_STATE_SUSPENDED) {
                lock->m_state = CNGSLock::LOCK_STATE_RESUMED;
            }
        }
    }
}

unsigned int CNGSLockManager::requestSharedData(int collectionType)
{
    CVector<int> collectionTypes;
    collectionTypes.Add(collectionType);
    return CAttributeManager::GetInstance()->getDataFromServer(collectionTypes, NULL, NULL);
}

unsigned int CNGSLockManager::saveSharedData(CNGSAttribute* attribute)
{
    CVector<CNGSAttribute*> attributes;
    attributes.Add(attribute);
    return CAttributeManager::GetInstance()->saveDataToServer(attributes, NULL, NULL, 0);
}
