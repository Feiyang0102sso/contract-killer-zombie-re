// Win32 copy of the recovered query interface using the ARM virtual table.
#include "ArmCall.h"
// PROVISIONAL (created by b057x): header-only Swerve C++ wrapper (swerve_tw), no owning unit
// (first weak copy in src/FPSGameplay/AI/CGameAIMap_NavMesh.cpp)
// twb_queryinterface.h - reference counted interface pointer of the Swerve object model.
#ifndef TWB_QUERYINTERFACE_H
#define TWB_QUERYINTERFACE_H

#include <stddef.h>

/** Base interface of the Swerve runtime objects (COM style reference counting). */
class IQueryInterface {
public:
    void AddRef() { return ckz::arm::Method<void>(this, 0); }
    void Release() { return ckz::arm::Method<void>(this, 1); }
    /** Interface `interfaceId` of this object (a new reference) in `*object`, NULL when unsupported. */  // added by b051x
    int QueryInterface(int interfaceId, void** object) { return ckz::arm::Method<int>(this, 2, interfaceId, object); }  // added by b051x
};

namespace swerve {

/** Owns one reference of an interface; ErrorHandler only selects the error policy. */
template <class ErrorHandler>
class QueryInterface {
public:
    QueryInterface()
    {
        m_interface = NULL;
    }

    /** Take over the reference held by `object` (no AddRef: getters hand out new references). */  // added by b051x
    explicit QueryInterface(IQueryInterface* object)  // added by b051x
    {  // added by b051x
        m_interface = object;  // added by b051x
    }  // added by b051x

    /** Share the reference of `other`. */  // added by b051x
    QueryInterface(const QueryInterface& other)  // added by b051x
    {  // added by b051x
        m_interface = other.m_interface;  // added by b051x
        AddReference(m_interface);  // added by b051x
    }  // added by b051x

    /** AddRef unless NULL (a helper: inlined it gives the original cmp + beq instead of cbz). */  // added by b051x
    static void AddReference(IQueryInterface* object)  // added by b051x
    {  // added by b051x
        if (object != NULL) {  // added by b051x
            object->AddRef();  // added by b051x
        }  // added by b051x
    }  // added by b051x

    /** Drop the reference (the handle becomes NULL). */  // added by b051x
    void Clear()  // added by b051x
    {  // added by b051x
        if (m_interface != NULL) {  // added by b051x
            m_interface->Release();  // added by b051x
        }  // added by b051x
        m_interface = NULL;  // added by b051x
    }  // added by b051x

    ~QueryInterface()
    {
        if (m_interface != NULL) {
            m_interface->Release();
        }
    }

    /** Take a new reference of `object` and drop the old one. */
    QueryInterface& operator=(IQueryInterface* object)
    {
        AddReference(object);  // changed by fx2 (helper: original cmp + beq, b051x experiment)
        if (m_interface != NULL) {
            m_interface->Release();
        }
        m_interface = object;
        return *this;
    }

    /** Share the reference of `other`. */
    QueryInterface& operator=(const QueryInterface& other)
    {
        return operator=(other.m_interface);
    }

    operator IQueryInterface*() const
    {
        return m_interface;
    }

    IQueryInterface* m_interface;   // 0x00
};

} // namespace swerve

#endif
