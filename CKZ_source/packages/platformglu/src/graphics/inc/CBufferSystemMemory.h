// CBufferSystemMemory.h - buffer data kept in system memory (created by b037x).
// Mixed into the system memory buffer classes (CIndexBuffer) next to their interface.
#ifndef CBUFFERSYSTEMMEMORY_H
#define CBUFFERSYSTEMMEMORY_H

#include <stddef.h>
#include "ICBuffer.h"

namespace com { namespace glu { namespace platform { namespace graphics {

class CBufferSystemMemory {
public:
    /** Bytes of one element of `type` (0 for unknown types). */
    static inline int GetComponentSize(unsigned int type)
    {
        switch (type) {
        case ComponentType_Byte:
        case ComponentType_UnsignedByte:
            return 1;
        case ComponentType_Short:
        case ComponentType_UnsignedShort:
            return 2;
        case ComponentType_Float:
        case ComponentType_Fixed:
        case ComponentType_Int:
        case ComponentType_UnsignedInt:
            return 4;
        }
        return 0;
    }

    int m_size;                 // +0x00 bytes allocated
    int m_stride;               // +0x04 bytes per element
    int m_length;               // +0x08 elements
    unsigned char* m_data;      // +0x0c
};

} } } }

#endif
