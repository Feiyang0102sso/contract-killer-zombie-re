// ICVertexBuffer.h - vertex buffer interface (created by b037x). Factory in the platform layer
// (cocoa/src/CGraphicsAbstractionManager_Cocoa.cpp).
#ifndef ICVERTEXBUFFER_H
#define ICVERTEXBUFFER_H

#include "ICBuffer.h"
#include "CVertex.h"

namespace com { namespace glu { namespace platform { namespace graphics {

class ICVertexBuffer : public ICBuffer {
public:
    /** Resource type key of vertex buffers. */
    enum { ResourceType = 0x26850270 };

    /** Cursor over a locked range of vertices. */
    struct Itr {
        const CVertex::Decl* m_decl;    // 0x00
        unsigned char* m_begin;         // 0x04
        unsigned char* m_current;       // 0x08
        int m_size;                     // 0x0c bytes in the range
        int m_stride;                   // 0x10 bytes per vertex
    };

    /** Vertex buffer of the current abstraction (NULL when there is none). */
    static ICVertexBuffer* CreateInstance();

    explicit ICVertexBuffer(unsigned int resourceType) : ICBuffer(resourceType) {}

    virtual ~ICVertexBuffer() {}

    using ICBuffer::Lock;

    /** `count` vertices laid out by `decl`; `stride` 0 = computed from the declaration. Returns 1. */
    virtual unsigned char Initialize(const CVertex::Attribute::Decl* decl, int count, int stride) = 0;
    virtual void Lock(Itr& itr, int first, int count, LockType lockType) = 0;
    virtual CVertex::Decl* GetDecl() = 0;
};

} } } }

#endif
