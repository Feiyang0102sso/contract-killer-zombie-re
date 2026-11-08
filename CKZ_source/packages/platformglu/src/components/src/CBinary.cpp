// CBinary.cpp - binary blob loaded from a stream.
#include "CBinary.h"
#include "CInputStream.h"

namespace com { namespace glu { namespace platform { namespace components {

CBinary::~CBinary()
{
    Destroy();
}

void CBinary::Destroy()
{
    if (m_data != NULL) {
        free(m_data);
    }
    m_data = NULL;
    m_size = 0;
    m_type = 0;
}

unsigned char CBinary::Load(CInputStream& stream, unsigned int type)
{
    unsigned int size = stream.Available();
    Destroy();
    if (size == 0) {
        return 0;
    }
    m_data = (unsigned char*)malloc(size);
    if (m_data != NULL) {
        stream.Read(m_data, size);
        if (!stream.m_error) {
            m_size = size;
            m_type = type;
            return 1;
        }
    }
    Destroy();
    return 0;
}

} } } }
