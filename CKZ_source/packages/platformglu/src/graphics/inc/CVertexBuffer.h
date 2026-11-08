// CVertexBuffer.h - vertex buffer in system memory.
// (first drafted by b002 as PROVISIONAL; completed by b040x, the owner of CVertexBuffer.cpp)
#ifndef CVERTEXBUFFER_H
#define CVERTEXBUFFER_H

#include "ICVertexBuffer.h"
#include "CBufferSystemMemory.h"
#include "CVertex.h"

namespace com { namespace glu { namespace platform { namespace graphics {

class CVertexBuffer : public ICVertexBuffer, public CBufferSystemMemory {
public:
    /** Resource type passed to ICGraphicsResource. */
    static const unsigned int ResourceType = 0x26850270;

    /** Room for 5 attributes, growing by 5 (the buffer itself is allocated by Initialize). */
    CVertexBuffer() : ICVertexBuffer(ResourceType)
    {
    }

    virtual ~CVertexBuffer();

    virtual ICGraphicsAbstract::Abstraction GetAbstraction() const;
    virtual void Destroy();
    virtual void Lock(unsigned char** data, int& size, int offset, int count, LockType lockType);
    virtual Type GetType();
    virtual int GetLength();
    virtual int GetStride();
    virtual int GetSize();
    virtual unsigned char Initialize(const CVertex::Attribute::Decl* decl, int count, int stride);
    virtual void Lock(Itr& itr, int first, int count, LockType lockType);
    virtual CVertex::Decl* GetDecl();

    CVertex::Decl m_decl;       // 0x18 attribute list
};

} } } }

#endif
