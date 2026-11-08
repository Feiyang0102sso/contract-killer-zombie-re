// CIdToObjectRouter.h - resolves resource keys / names through user supplied callbacks
// (created by b005, header-only).
#ifndef CIDTOOBJECTROUTER_H
#define CIDTOOBJECTROUTER_H

#include <stddef.h>  // added by b011x
#include "CClass.h"

namespace com { namespace glu { namespace platform {

namespace core {
template <class T> class CSharedPtr;
}

namespace components {

class CInputStream;

class CIdToObjectRouter : public core::CClass {
public:
    typedef unsigned char (*KeyToNameFunc)(void* context, unsigned int key, char** outName);
    typedef unsigned char (*KeyToPointerFunc)(void* context, unsigned int key, core::CSharedPtr<void*>& outPointer, unsigned int& outType);
    typedef unsigned char (*KeyToInputStreamFunc)(void* context, unsigned int key, CInputStream& outStream, unsigned int& outType);
    typedef unsigned char (*NameToPointerFunc)(void* context, const char* name, core::CSharedPtr<void*>& outPointer, unsigned int& outType);
    typedef unsigned char (*NameToInputStreamFunc)(void* context, const char* name, CInputStream& outStream, unsigned int& outType);

    /** Runtime class id. */  // added by b011x
    enum { ClassId = 0xb063b7b6 };  // added by b011x

    /** No callbacks installed. */  // added by b011x
    CIdToObjectRouter()  // added by b011x
    {  // added by b011x
        m_classId = ClassId;  // added by b011x
        m_keyToNameContext = NULL;  // added by b011x
        m_keyToName = NULL;  // added by b011x
        m_keyToPointerContext = NULL;  // added by b011x
        m_keyToPointer = NULL;  // added by b011x
        m_keyToInputStreamContext = NULL;  // added by b011x
        m_keyToInputStream = NULL;  // added by b011x
        m_nameToPointerContext = NULL;  // added by b011x
        m_nameToPointer = NULL;  // added by b011x
        m_nameToInputStreamContext = NULL;  // added by b011x
        m_nameToInputStream = NULL;  // added by b011x
    }  // added by b011x

    virtual ~CIdToObjectRouter() {}

    virtual unsigned char KeyToName(unsigned int key, char** outName)
    {
        if (m_keyToName != NULL) {
            return m_keyToName(m_keyToNameContext, key, outName);
        }
        return 0;
    }

    virtual unsigned char KeyToPointer(unsigned int key, core::CSharedPtr<void*>& outPointer, unsigned int& outType)
    {
        if (m_keyToPointer != NULL) {
            return m_keyToPointer(m_keyToPointerContext, key, outPointer, outType);
        }
        return 0;
    }

    virtual unsigned char KeyToInputStream(unsigned int key, CInputStream& outStream, unsigned int& outType)
    {
        if (m_keyToInputStream != NULL) {
            return m_keyToInputStream(m_keyToInputStreamContext, key, outStream, outType);
        }
        return 0;
    }

    virtual unsigned char NameToPointer(const char* name, core::CSharedPtr<void*>& outPointer, unsigned int& outType)
    {
        if (m_nameToPointer != NULL) {
            return m_nameToPointer(m_nameToPointerContext, name, outPointer, outType);
        }
        return 0;
    }

    virtual unsigned char NameToInputStream(const char* name, CInputStream& outStream, unsigned int& outType)
    {
        if (m_nameToInputStream != NULL) {
            return m_nameToInputStream(m_nameToInputStreamContext, name, outStream, outType);
        }
        return 0;
    }

    void* m_keyToNameContext;                       // 0x08
    KeyToNameFunc m_keyToName;                      // 0x0c
    void* m_keyToPointerContext;                    // 0x10
    KeyToPointerFunc m_keyToPointer;                // 0x14
    void* m_keyToInputStreamContext;                // 0x18
    KeyToInputStreamFunc m_keyToInputStream;        // 0x1c
    void* m_nameToPointerContext;                   // 0x20
    NameToPointerFunc m_nameToPointer;              // 0x24
    void* m_nameToInputStreamContext;               // 0x28
    NameToInputStreamFunc m_nameToInputStream;      // 0x2c
};

} } } }

#endif
