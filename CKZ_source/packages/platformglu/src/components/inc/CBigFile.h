// CBigFile.h - abstract resource archive ("big file") (created by b005, header-only).
#ifndef CBIGFILE_H
#define CBIGFILE_H

#include <wchar.h>
#include "CClass.h"

namespace com { namespace glu { namespace platform { namespace components {

class CInputStream;

class CBigFile : public core::CClass {
public:
    enum { ClassId = 0x30faff1d };

    CBigFile()
    {
        m_classId = ClassId;
        memset(m_signature, 0, sizeof(m_signature));
        m_versionMajor = 0;
        m_versionMinor = 0;
        m_versionRevision = 0;
    }

    virtual ~CBigFile() {}

    virtual void Destroy() = 0;
    /** Read the archive directory from `stream`. */
    virtual unsigned char Load(CInputStream& stream, unsigned int size) = 0;
    virtual unsigned char Load(const wchar_t* path) = 0;
    /** Mime key of a resource by name hash / by name, 0 when unknown. */
    virtual unsigned int GetMimeKey(unsigned int nameKey) = 0;
    virtual unsigned int GetMimeKey(const char* name) = 0;

    char m_signature[5];                // 0x08 4 signature chars + terminator
    unsigned char m_versionMajor;       // 0x0d
    unsigned char m_versionMinor;       // 0x0e
    unsigned char m_versionRevision;    // 0x0f
};

} } } }

#endif
