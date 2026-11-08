// ICIndexBuffer.h - index buffer interface (created by b037x). Factory in the platform layer
// (cocoa/src/CGraphicsAbstractionManager_Cocoa.cpp).
#ifndef ICINDEXBUFFER_H
#define ICINDEXBUFFER_H

#include "ICBuffer.h"

namespace com { namespace glu { namespace platform { namespace graphics {

class ICIndexBuffer : public ICBuffer {
public:
    /** Resource type key of index buffers. */
    enum { ResourceType = 0x2cce7fa0 };

    /** Cursor over a locked range of indices. */
    struct Itr {
        unsigned int m_indexType;   // 0x00 ComponentType of the indices
        unsigned char* m_begin;     // 0x04
        unsigned char* m_current;   // 0x08
        int m_size;                 // 0x0c bytes in the range
        int m_stride;               // 0x10 bytes per index
    };

    /** Index buffer of the current abstraction (NULL when there is none). */
    static ICIndexBuffer* CreateInstance();

    explicit ICIndexBuffer(unsigned int resourceType) : ICBuffer(resourceType) {}

    virtual ~ICIndexBuffer() {}

    using ICBuffer::Lock;

    /** `count` indices of `indexType`; `stride` 0 = size of the type. Returns 1. */
    virtual unsigned char Initialize(unsigned int indexType, int count, int stride) = 0;
    virtual void Lock(Itr& itr, int first, int count, LockType lockType) = 0;
    virtual unsigned int GetIndexType() = 0;
};

} } } }

#endif
