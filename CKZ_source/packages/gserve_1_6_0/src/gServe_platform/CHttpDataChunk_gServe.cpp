// CHttpDataChunk_gServe.cpp - chunk of a chunked HTTP response.
#include "CHttpDataChunk_gServe.h"

CHttpDataChunk_gServe::CHttpDataChunk_gServe()
{
    m_data = NULL;
    m_size = 0;
    m_received = 0;
}

CHttpDataChunk_gServe::~CHttpDataChunk_gServe()
{
    if (m_data != NULL) {
        free(m_data);
        m_data = NULL;
    }
}
