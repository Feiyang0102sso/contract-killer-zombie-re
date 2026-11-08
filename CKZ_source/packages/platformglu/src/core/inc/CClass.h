// CClass.h - root of the Glu runtime-typed classes (created by b005, header-only).
// Every CClass carries a 32-bit class id right after its vtable pointer; pools, singletons
// and resource tables use the id as a key.
#ifndef CCLASS_H
#define CCLASS_H

namespace com { namespace glu { namespace platform { namespace core {

class CClass {
public:
    CClass() {}  // added by b006
    /** Set the class id before any member of the derived class is constructed. */  // added by b006
    explicit CClass(unsigned int classId) : m_classId(classId) {}  // added by b006

    virtual ~CClass() {}

    unsigned int m_classId;     // 0x04 runtime class id
};

} } } }

#endif
