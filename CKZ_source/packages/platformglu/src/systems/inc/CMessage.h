// CMessage.h - event carrying typed arguments; messages live in the application CMessagePool.
// (first drafted by b005 as PROVISIONAL; completed by b011x, the owner)
#ifndef CMESSAGE_H
#define CMESSAGE_H

#include <stddef.h>
#include "CEvent.h"
#include "CPool.h"
#include "CSingleton.h"

namespace com { namespace glu { namespace platform { namespace systems {

/** Pool all CMessage objects are allocated from (application singleton). */
class CMessagePool : public components::CPool, public components::CSingleton {
public:
    /** Key in the application singleton table: CStringToKey("CMessagePool"). */
    enum { SingletonId = 0x571812b8 };

    CMessagePool() : components::CSingleton(SingletonId) {}

    virtual ~CMessagePool();

    /** The application's pool, created on first use. */
    static inline CMessagePool* GetInstance()
    {
        components::CSingleton* found = NULL;
        CApplet::m_pApp->m_singletons->Find(SingletonId, (void**)&found);
        CMessagePool* pool = static_cast<CMessagePool*>(found);
        if (pool == NULL) {
            pool = new CMessagePool();
        }
        return pool;
    }
};

inline CMessagePool::~CMessagePool()
{
}

class CMessage : public CEvent {
public:
    /** Runtime class id: CStringToKey("CMessage"). */
    enum { ClassId = 0x7b245719 };

    /** Type key of a 32-bit unsigned argument: CStringToKey("uint32"). */
    enum { ArgumentType_UInt32 = 0x013f9702 };

    /** Most arguments a message carries. */
    enum { MaxArguments = 25 };

    /** One argument: a type key, an element count (0 = the value itself is in m_data) and the data. */
    struct Argument {
        unsigned int m_type;    // 0x00
        unsigned int m_count;   // 0x04
        void* m_data;           // 0x08
    };

    /** `argumentCount` (type, count, data) triples follow as variadic arguments. */
    CMessage(core::CClass* sender, unsigned int messageId, unsigned int param, int argumentCount, ...);

    virtual ~CMessage() {}

    static inline void* operator new(size_t size)
    {
        return CMessagePool::GetInstance()->New(size);
    }

    static inline void operator delete(void* message)
    {
        CMessagePool::GetInstance()->Delete((core::CClass*)message);
    }

    int m_argumentCount;                    // 0x2c
    Argument m_arguments[MaxArguments];     // 0x30
};

} } } }

#endif
