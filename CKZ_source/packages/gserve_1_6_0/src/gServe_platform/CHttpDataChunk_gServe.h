// CHttpDataChunk_gServe.h - one received block of a chunked HTTP response (kept in a CLinkList).
// gserve's copy of network::CHttpDataChunk.
#ifndef CHTTPDATACHUNK_GSERVE_H
#define CHTTPDATACHUNK_GSERVE_H

#include "CLinkList.h"

class CHttpDataChunk_gServe : public com::glu::platform::core::CLinkListNode {
public:
    CHttpDataChunk_gServe();
    virtual ~CHttpDataChunk_gServe();

    unsigned char* m_data;          // 0x14 chunk payload (malloc'd, m_size bytes)
    unsigned int m_size;            // 0x18 size announced by the chunk header
    unsigned int m_received;        // 0x1c bytes of the payload received so far
};

#endif
