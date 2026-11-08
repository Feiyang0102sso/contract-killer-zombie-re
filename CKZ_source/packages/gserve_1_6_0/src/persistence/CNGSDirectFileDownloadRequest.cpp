// CNGSDirectFileDownloadRequest.cpp - a queued direct file download (b025x).
#include "CNGSDirectFileDownloadRequest.h"
#include "CMD5_gServe.h"

using com::glu::platform::components::CStrWChar;

CNGSDirectFileDownloadRequest::CNGSDirectFileDownloadRequest(CStrWChar fileName, CStrWChar url, unsigned int crc,
                                                             CStrWChar dataPath,
                                                             eCNGSDirectFileDownloadRequestType type,
                                                             eCNGSDirectFileDownloadDownloadControl control)
{
    m_fileName = fileName;
    m_url = url;
    m_type = type;
    m_control = control;
    m_crc = crc;
    m_dataPath = dataPath;
    m_status = DOWNLOAD_STATUS_NONE;
    m_downloadedCount = 0;
    m_batchCount = 0;
    m_batchName = L"";

    // The cache name is the MD5 of the file name (the digest buffer is never freed, as in the original).
    char* name = new char[m_fileName.GetLength() + 1];
    wcstombs(name, m_fileName.GetWCharPtr(), wcslen(m_fileName.GetWCharPtr()) + 1);
    m_cacheName += CMD5_gServe::GetInstance()->GenerateMD5(name);
    delete[] name;
}

CNGSDirectFileDownloadRequest::~CNGSDirectFileDownloadRequest()
{
}
