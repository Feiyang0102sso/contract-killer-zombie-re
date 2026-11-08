// CBH_PersonTypeManager.h - all person types of res/common/configs/persons.xml by name.
#ifndef CBH_PERSONTYPEMANAGER_H
#define CBH_PERSONTYPEMANAGER_H

#include <stddef.h>
#include "xString.h"
#include "Vector_ss.h"
#include "HashTable.h"

class CBH_PersonType;

class CBH_PersonTypeManager {
public:
    CBH_PersonTypeManager();
    virtual ~CBH_PersonTypeManager();

    /** The type named `name` (must exist). */
    CBH_PersonType* GetType(const XString& name) const;
    bool TypeIsExists(const XString& name) const;

    HashTable<XString, CBH_PersonType*> m_types;    // 0x04
    Vector<XString> m_typeNames;                    // 0x10 in file order
};

#endif
