// CIndexBuffer.h - index buffer in system memory.
// (first drafted by b002 as PROVISIONAL; completed by b037x, the owner)
#ifndef CINDEXBUFFER_H
#define CINDEXBUFFER_H

#include "ICIndexBuffer.h"
#include "CBufferSystemMemory.h"

namespace com { namespace glu { namespace platform { namespace graphics {

class CIndexBuffer : public ICIndexBuffer, public CBufferSystemMemory {
public:
    /** Runtime class id (resource type of index buffers). */
    static const unsigned int ClassId = 0x2cce7fa0;

    CIndexBuffer() : ICIndexBuffer(ClassId) {}
    virtual ~CIndexBuffer();

    virtual ICGraphicsAbstract::Abstraction GetAbstraction() const;
    virtual void Destroy();
    virtual void Lock(unsigned char** data, int& size, int offset, int count, LockType lockType);
    virtual Type GetType();
    virtual int GetLength();
    virtual int GetStride();
    virtual int GetSize();
    virtual unsigned char Initialize(unsigned int indexType, int count, int stride);
    virtual void Lock(Itr& itr, int first, int count, LockType lockType);
    virtual unsigned int GetIndexType();

    unsigned int m_indexType;   // 0x18 ComponentType of the indices
};

} } } }

#endif
