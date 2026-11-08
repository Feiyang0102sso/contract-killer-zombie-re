// CNGSDirectFileDownloadRequest.h - one file queued in CNGSDirectFileDownload (b025x).
#ifndef CNGSDIRECTFILEDOWNLOADREQUEST_H
#define CNGSDIRECTFILEDOWNLOADREQUEST_H

#include "CStrWChar.h"

/** How the server delivers the file (selects the URL folder and the response handler). */
enum eCNGSDirectFileDownloadRequestType {
    DIRECT_FILE_DOWNLOAD_BATCH = 0,         // "/batch/": control file listing more files
    DIRECT_FILE_DOWNLOAD_ENCODED = 1,       // "/encoded/": JSON with base64 data
    DIRECT_FILE_DOWNLOAD_DATA = 2,          // "/data/": raw bytes
    DIRECT_FILE_DOWNLOAD_ENCODED_3 = 3,     // "/encoded/" as well
    DIRECT_FILE_DOWNLOAD_CONTENT = 4        // "/content/": raw bytes
};

/** When the file is fetched again. */
enum eCNGSDirectFileDownloadDownloadControl {
    DOWNLOAD_CONTROL_IF_NOT_CACHED = 0,     // only when not in the cache
    DOWNLOAD_CONTROL_IF_MODIFIED = 1,       // conditional GET against the cached copy
    DOWNLOAD_CONTROL_ALWAYS = 2,
    DOWNLOAD_CONTROL_IF_CRC_DIFFERS = 3     // when the cached copy has another CRC
};

/** Progress of a request (m_status). */
enum eCNGSDirectFileDownloadDownloadStatus {
    DOWNLOAD_STATUS_NONE = 0,
    DOWNLOAD_STATUS_QUEUED = 1,
    DOWNLOAD_STATUS_DOWNLOADING = 2,
    DOWNLOAD_STATUS_COMPLETE = 3,
    DOWNLOAD_STATUS_CACHED = 4,
    DOWNLOAD_STATUS_CRC_ERROR = 5,
    DOWNLOAD_STATUS_NOT_MODIFIED = 6,
    DOWNLOAD_STATUS_EMPTY = 7
};

class CNGSDirectFileDownloadRequest {
public:
    CNGSDirectFileDownloadRequest(com::glu::platform::components::CStrWChar fileName,
                                  com::glu::platform::components::CStrWChar url,
                                  unsigned int crc,
                                  com::glu::platform::components::CStrWChar dataPath,
                                  eCNGSDirectFileDownloadRequestType type,
                                  eCNGSDirectFileDownloadDownloadControl control);
    ~CNGSDirectFileDownloadRequest();

    com::glu::platform::components::CStrWChar m_url;            // +0x00 absolute ("http://") or relative to the folder URL
    com::glu::platform::components::CStrWChar m_unused10;       // +0x10 never set in gserve
    com::glu::platform::components::CStrWChar m_cacheName;      // +0x20 MD5 of the file name
    com::glu::platform::components::CStrWChar m_fileName;       // +0x30 name in the cache folder
    com::glu::platform::components::CStrWChar m_dataPath;       // +0x40 JSON path of the data (encoded files)
    unsigned int m_crc;                                         // +0x50 expected CRC-32 (0 = unknown)
    com::glu::platform::components::CStrWChar m_batchName;      // +0x54 batch this file belongs to
    int m_downloadedCount;                                      // +0x64 batch files done
    int m_batchCount;                                           // +0x68 batch files in total
    eCNGSDirectFileDownloadRequestType m_type;                  // +0x6c
    eCNGSDirectFileDownloadDownloadControl m_control;           // +0x70
    eCNGSDirectFileDownloadDownloadStatus m_status;             // +0x74
};

#endif
