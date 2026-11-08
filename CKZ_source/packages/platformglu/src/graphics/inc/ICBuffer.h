// ICBuffer.h - vertex / index data buffer interface (created by b037x).
// Slot order from the vtables of CIndexBuffer and CVertexBuffer.
#ifndef ICBUFFER_H
#define ICBUFFER_H

#include "ICGraphicsResource.h"

namespace com { namespace glu { namespace platform { namespace graphics {

/** Element types of buffer data (index types and vertex attribute component types). */
enum ComponentType {
    ComponentType_Undefined     = 0,
    ComponentType_Byte          = 0x0002f978,
    ComponentType_UnsignedByte  = 0x0023f978,
    ComponentType_Float         = 0x0030a964,
    ComponentType_Fixed         = 0x0030ee34,   // 16.16 fixed point
    ComponentType_Int           = 0x003f9702,
    ComponentType_Short         = 0x003f9726,
    ComponentType_UnsignedInt   = 0x013f9702,
    ComponentType_UnsignedShort = 0x013f9726
};

class ICBuffer : public ICGraphicsResource {
public:
    /** GetType() values. */
    enum Type {
        Type_Index = 0,
        Type_Vertex = 1
    };

    /** How a locked range is going to be used. */
    enum LockType {
        LockType_ReadWrite = 0,
        LockType_Read = 1,
        LockType_Write = 2
    };

    /** A range of a buffer bound as a vertex / index stream. */
    struct Stream {
        ICBuffer* m_buffer;         // 0x00
        unsigned int m_first;       // 0x04 first element
        unsigned int m_count;       // 0x08 elements
        unsigned char m_dirty;      // 0x0c changed since the device last used it

        // user-declared: CVector copies streams without zero-filling the slot first
        Stream() {}
    };

    explicit ICBuffer(unsigned int resourceType) : ICGraphicsResource(resourceType) {}

    virtual ~ICBuffer() {}

    virtual void Destroy() = 0;
    /** Map `count` bytes from byte `offset`; *data is NULL when nothing is available. */
    virtual void Lock(unsigned char** data, int& size, int offset, int count, LockType lockType) = 0;
    virtual Type GetType() = 0;
    /** Elements in the buffer. */
    virtual int GetLength() = 0;
    /** Bytes per element. */
    virtual int GetStride() = 0;
    /** Bytes in the buffer. */
    virtual int GetSize() = 0;
};

} } } }

#endif
