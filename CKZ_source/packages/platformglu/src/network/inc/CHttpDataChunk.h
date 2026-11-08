// CHttpDataChunk.h - one received block of a chunked HTTP response (kept in a CLinkList).
#ifndef CHTTPDATACHUNK_H
#define CHTTPDATACHUNK_H

#include "CLinkList.h"

namespace com { namespace glu { namespace platform { namespace network {

class CHttpDataChunk : public core::CLinkListNode {
public:
    CHttpDataChunk();
    virtual ~CHttpDataChunk();

    unsigned char* m_data;          // 0x14 chunk payload (malloc'd, m_size bytes)
    unsigned int m_size;            // 0x18 size announced by the chunk header
    unsigned int m_received;        // 0x1c bytes of the payload received so far
};

} } } }

#endif
