// CBinary.h - a block of binary data loaded from a stream, tagged with its type (mime key).
#ifndef CBINARY_H
#define CBINARY_H

#include "CClass.h"

namespace com { namespace glu { namespace platform { namespace components {

class CInputStream;

class CBinary : public core::CClass {
public:
    enum { ClassId = 0x374f8659 };

    CBinary()
    {
        m_classId = ClassId;
        m_data = NULL;
        m_size = 0;
        m_type = 0;
    }

    virtual ~CBinary();

    /** Read everything left in `stream`; `type` is stored as the data type. */
    virtual unsigned char Load(CInputStream& stream, unsigned int type);
    virtual void Destroy();

    unsigned char* m_data;          // 0x08 owned buffer
    unsigned int m_size;            // 0x0c
    unsigned int m_type;            // 0x10 mime key of the content
};

} } } }

#endif
