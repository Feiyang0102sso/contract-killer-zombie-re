// CNGSDirectFileDownload.cpp - queued direct file downloads into the application cache (b025x).
#include "CNGSDirectFileDownload.h"
#include "CNGS.h"
#include "CNGSUtil.h"
#include "CNGSJSONParser.h"
#include "CFileUtil_gServe.h"
#include "CCRC32_gServe.h"
#include "CNotificationHandler.h"
#include "ICFileMgr.h"
#include "ICStdUtil.h"
#include "CGenUtil.h"

using com::glu::platform::components::CStrWChar;
using com::glu::platform::components::ICFileMgr;
using com::glu::platform::core::CVector;
using com::glu::platform::core::ICStdUtil;
using com::glu::platform::core::CGenUtil;

CNGSDirectFileDownload::CNGSDirectFileDownload()
    : CSingleton(ClassId)
{
    m_noSaveToDisk = true;
    m_downloading = false;
    CStrWChar folder("");
    SetFolder(folder);
    m_notificationHandler = NULL;
}

void CNGSDirectFileDownload::SetFolder(CStrWChar& folder)
{
    m_folder = L"";
    CStrWChar cachePath;
    CFileUtil_gServe::GetApplicationCachePath(cachePath);
    cachePath += ICFileMgr::GetInstance()->GetPathSeparator();
    CNGSUtil::IntegerToString(CNGS::GetInstance()->m_gameId, m_folder);
    if (folder.GetLength() > 0) {
        if (wcsncmp(folder.GetWCharPtr(), ICFileMgr::GetInstance()->GetPathSeparator(), 1) != 0) {
            m_folder += ICFileMgr::GetInstance()->GetPathSeparator();
        }
        m_folder += folder.GetWCharPtr();
    }
    cachePath += m_folder.GetWCharPtr();
    ICFileMgr::GetInstance()->MkDir(cachePath.GetWCharPtr());

    CStrWChar dataFolderName(L"data");
    m_dataFolder = dataFolderName;
    m_dataFolder += ICFileMgr::GetInstance()->GetPathSeparator();
    m_dataFolder += m_folder.GetWCharPtr();
}

CNGSDirectFileDownload::~CNGSDirectFileDownload()
{
    for (int index = 0; index < m_queue.Size(); index++) {
        if (m_queue[index] != NULL) {
            delete m_queue[index];
            m_queue[index] = NULL;
        }
    }
    m_queue.Clear();
    for (int index = 0; index < m_completed.Size(); index++) {
        if (m_completed[index] != NULL) {
            delete m_completed[index];
            m_completed[index] = NULL;
        }
    }
    m_completed.Clear();
}

const wchar_t* CNGSDirectFileDownload::GetFolder()
{
    return m_folder.GetWCharPtr();
}

void CNGSDirectFileDownload::AddToDownloadQueue(CNGSDirectFileDownloadRequest* request)
{
    request->m_status = DOWNLOAD_STATUS_QUEUED;
    m_queue.Add(request);
}

void CNGSDirectFileDownload::StartNextDownload()
{
    m_downloading = true;
    if (m_queue.Size() == 0) {
        return;
    }
    CStrWChar baseURL(m_dataFolder);
    m_currentRequest = m_queue[0];
    m_currentRequest->m_status = DOWNLOAD_STATUS_DOWNLOADING;

    // The request type selects the server folder and the response handler.
    switch (m_currentRequest->m_type) {
    case DIRECT_FILE_DOWNLOAD_DATA:
        SetRequestCallback(new CNGSDirectFileDownloadRequestFunctor(this, &CNGSDirectFileDownload::HandleDataResponse));
        baseURL += L"/data/";
        break;
    case DIRECT_FILE_DOWNLOAD_ENCODED:
    case DIRECT_FILE_DOWNLOAD_ENCODED_3:
        SetRequestCallback(new CNGSDirectFileDownloadRequestFunctor(this, &CNGSDirectFileDownload::HandleEncodedResponse));
        baseURL += L"/encoded/";
        break;
    case DIRECT_FILE_DOWNLOAD_BATCH:
        SetRequestCallback(new CNGSDirectFileDownloadRequestFunctor(this, &CNGSDirectFileDownload::HandleBatchResponse));
        baseURL += L"/batch/";
        break;
    case DIRECT_FILE_DOWNLOAD_CONTENT:
        SetRequestCallback(new CNGSDirectFileDownloadRequestFunctor(this, &CNGSDirectFileDownload::HandleDataResponse));
        baseURL += L"/content/";
        break;
    default:
        break;
    }

    switch (m_currentRequest->m_control) {
    case DOWNLOAD_CONTROL_IF_NOT_CACHED:
        if (!CFileUtil_gServe::IsFileInCache(m_folder.GetWCharPtr(), m_currentRequest->m_fileName.GetWCharPtr())) {
            if (wcsncmp(L"http://", m_currentRequest->m_url.GetWCharPtr(), 7) == 0) {
                LoadFromServer(m_currentRequest->m_url, true);
            } else {
                LoadFromServer(baseURL + m_currentRequest->m_url, true);
            }
        } else {
            AddToCompletedList(m_currentRequest, DOWNLOAD_STATUS_CACHED);
            DownloadComplete(m_currentRequest);
        }
        break;
    case DOWNLOAD_CONTROL_IF_MODIFIED:
        if (CFileUtil_gServe::IsFileInCache(m_folder.GetWCharPtr(), m_currentRequest->m_fileName.GetWCharPtr())) {
            int modificationDate = CFileUtil_gServe::GetModificationDateFromCache(m_folder.GetWCharPtr(),
                                                                                  m_currentRequest->m_fileName.GetWCharPtr());
            SetLastAccessTime(modificationDate);
        }
        if (wcsncmp(L"http://", m_currentRequest->m_url.GetWCharPtr(), 7) == 0) {
            LoadFromServer(m_currentRequest->m_url, false);
        } else {
            LoadFromServer(baseURL + m_currentRequest->m_url, false);
        }
        AddToCompletedList(m_currentRequest, DOWNLOAD_STATUS_QUEUED);
        break;
    case DOWNLOAD_CONTROL_ALWAYS:
        if (wcsncmp(L"http://", m_currentRequest->m_url.GetWCharPtr(), 7) != 0) {
            LoadFromServer(baseURL + m_currentRequest->m_url, true);
        } else {
            LoadFromServer(m_currentRequest->m_url, true);
        }
        break;
    case DOWNLOAD_CONTROL_IF_CRC_DIFFERS:
        if (CFileUtil_gServe::GetCRCFromFileInCache(m_folder.GetWCharPtr(), m_currentRequest->m_fileName.GetWCharPtr())
            != m_currentRequest->m_crc) {
            LoadFromServer(baseURL + m_currentRequest->m_url, true);
        } else {
            AddToCompletedList(m_currentRequest, DOWNLOAD_STATUS_CACHED);
            DownloadComplete(m_currentRequest);
        }
        break;
    }
    m_queue.RemoveAt(0);
}

void CNGSDirectFileDownload::HandleDataResponse(char* data, int length, bool notModified)
{
    if (notModified) {
        UpdateCompletedList(m_currentRequest, DOWNLOAD_STATUS_NOT_MODIFIED);
    } else {
        if (length == 0) {
            m_currentRequest->m_status = DOWNLOAD_STATUS_EMPTY;
        } else if (CCRC32_gServe::GetInstance()->GenerateCRC32((unsigned char*)data, length) != m_currentRequest->m_crc) {
            m_currentRequest->m_status = DOWNLOAD_STATUS_CRC_ERROR;
        } else {
            CFileUtil_gServe::SaveFileToCache(m_folder.GetWCharPtr(), m_currentRequest->m_fileName.GetWCharPtr(),
                                              (unsigned char*)data, length);
            m_currentRequest->m_status = DOWNLOAD_STATUS_COMPLETE;
        }
        // The original lists the request as COMPLETE whatever the result above.
        AddToCompletedList(m_currentRequest, DOWNLOAD_STATUS_COMPLETE);
    }
    m_busy = false;
    DownloadComplete(m_currentRequest);
}

void CNGSDirectFileDownload::HandleEncodedResponse(char* data, int length, bool notModified)
{
    if (notModified) {
        UpdateCompletedList(m_currentRequest, DOWNLOAD_STATUS_NOT_MODIFIED);
    } else {
        if (length == 0) {
            m_currentRequest->m_status = DOWNLOAD_STATUS_EMPTY;
        } else {
            char* text = new char[length + 1];
            memcpy(text, data, length);
            text[length] = 0;
            m_root = CNGSJSONParser::decodeValue(CStrWChar(text));

            CStrWChar fileName;
            FindStringAttribute(m_currentRequest->m_dataPath, CStrWChar(L"filename"), &fileName);
            void* decoded = NULL;
            int decodedLength = GetDecodedBinaryData(m_currentRequest->m_dataPath, CStrWChar(L"data"), &decoded);
            if (decoded != NULL && decodedLength > 0) {
                // The CRC is taken over the encoded document, as in the original.
                if (CCRC32_gServe::GetInstance()->GenerateCRC32((unsigned char*)text, length) != m_currentRequest->m_crc) {
                    m_currentRequest->m_status = DOWNLOAD_STATUS_CRC_ERROR;
                } else {
                    CFileUtil_gServe::SaveFileToCache(m_folder.GetWCharPtr(), m_currentRequest->m_fileName.GetWCharPtr(),
                                                      (unsigned char*)decoded, decodedLength);
                    m_currentRequest->m_status = DOWNLOAD_STATUS_COMPLETE;
                }
                free(decoded);
            }
            free(text);
            // Back to the document of the last batch file.
            m_root = CNGSJSONParser::decodeValue(CStrWChar(m_rawJSON));
        }
        AddToCompletedList(m_currentRequest, DOWNLOAD_STATUS_COMPLETE);
    }
    m_busy = false;
    DownloadComplete(m_currentRequest);
}

void CNGSDirectFileDownload::HandleBatchResponse(char* data, int length, bool notModified)
{
    if (notModified) {
        processControlFile();
        UpdateCompletedList(m_currentRequest, DOWNLOAD_STATUS_NOT_MODIFIED);
    } else {
        if (length == 0) {
            m_currentRequest->m_status = DOWNLOAD_STATUS_EMPTY;
        } else {
            HandleValidateCNGSJSONDataResponse(data, length, notModified);
            unsigned int crc = CCRC32_gServe::GetInstance()->GenerateCRC32((unsigned char*)data, length);
            if (m_currentRequest->m_crc != 0 && crc != m_currentRequest->m_crc) {
                m_currentRequest->m_status = DOWNLOAD_STATUS_CRC_ERROR;
            } else {
                CFileUtil_gServe::SaveFileToCache(m_folder.GetWCharPtr(), m_currentRequest->m_fileName.GetWCharPtr(),
                                                  (unsigned char*)data, length);
                processControlFile();
                m_currentRequest->m_status = DOWNLOAD_STATUS_COMPLETE;
            }
        }
        AddToCompletedList(m_currentRequest, DOWNLOAD_STATUS_COMPLETE);
    }
    DownloadComplete(m_currentRequest);
}

unsigned char CNGSDirectFileDownload::AddToCompletedList(CNGSDirectFileDownloadRequest* request,
                                                         eCNGSDirectFileDownloadDownloadStatus status)
{
    unsigned char listed = UpdateCompletedList(request, status);
    if (!listed) {
        m_completed.Add(request);
    }
    return listed;
}

void CNGSDirectFileDownload::DownloadComplete(CNGSDirectFileDownloadRequest* request)
{
    // Text of a log message that is compiled out.
    CStrWChar message;
    message += "Download of ";
    message += m_currentRequest->m_fileName.GetWCharPtr();
    message += " complete";
    if (m_notificationHandler != NULL) {
        bool success;
        switch (request->m_status) {
        case DOWNLOAD_STATUS_COMPLETE:
        case DOWNLOAD_STATUS_CACHED:
        case DOWNLOAD_STATUS_NOT_MODIFIED:
            success = true;
            break;
        case DOWNLOAD_STATUS_CRC_ERROR:
        case DOWNLOAD_STATUS_EMPTY:
            success = false;
            break;
        default:
            success = false;
            break;
        }
        m_notificationHandler->HandleFileDownloadedMessage(success, *request);
        IncrementDownloadCount(m_currentRequest);
    }
}

CVector<CNGSDirectFileDownloadRequest*> CNGSDirectFileDownload::GetQueued()
{
    return m_queue;
}

unsigned char CNGSDirectFileDownload::UpdateCompletedList(CNGSDirectFileDownloadRequest* request,
                                                          eCNGSDirectFileDownloadDownloadStatus status)
{
    request->m_status = status;
    for (int index = 0; index < m_completed.Size(); index++) {
        if (m_completed[index]->m_fileName == request->m_fileName) {
            m_completed[index]->m_status = status;
            return 1;
        }
    }
    return 0;
}

int CNGSDirectFileDownload::IncrementDownloadCount(const CNGSDirectFileDownloadRequest* request)
{
    if (request->m_batchName.GetLength() > 0) {
        for (int index = 0; index < m_completed.Size(); index++) {
            if (m_completed[index]->m_fileName == request->m_batchName) {
                m_completed[index]->m_downloadedCount++;
                int batchCount = m_completed[index]->m_batchCount;
                if (m_completed[index]->m_downloadedCount != batchCount) {
                    return 0;
                }
                if (batchCount > 0) {
                    m_notificationHandler->HandleBatchFileDownloadCompleteMessage(request->m_batchName, batchCount);
                }
                return batchCount;
            }
        }
    }
    return -1;
}

void CNGSDirectFileDownload::HandleUpdate(int elapsedMs)
{
    CNGSJSONData::HandleUpdate(elapsedMs);
    if (m_downloading) {
        if (!IsBusy()) {
            StartNextDownload();
        }
    }
}

int CNGSDirectFileDownload::GetDecodedBinaryData(CStrWChar path, CStrWChar name, void** data)
{
    int decodedLength = -1;
    CStrWChar encoded;
    if (FindStringAttribute(path, name, &encoded)) {
        char text[encoded.GetLength() + 1];
        memset(text, 0, encoded.GetLength() + 1);
        int textLength = encoded.GetLength();
        if (wcstombs(text, encoded.GetWCharPtr(), encoded.GetLength() + 1) == (size_t)-1) {
            text[0] = 0;
        } else {
            text[textLength] = 0;
        }
        unsigned char* buffer = (unsigned char*)malloc(encoded.GetLength());
        decodedLength = CNGSUtil::Base64Decode(buffer, (unsigned char*)text, encoded.GetLength());
        *data = malloc(decodedLength);
        memcpy(*data, buffer, decodedLength);
        free(buffer);
    }
    return decodedLength;
}

void CNGSDirectFileDownload::registerNotificationHandler(CNotificationHandler* handler)
{
    m_notificationHandler = handler;
}

unsigned char CNGSDirectFileDownload::processControlFile()
{
    CStrWChar fileName;
    bool queued = false;
    int index = 0;
    unsigned char found = 0;
    do {
        char key[15];
        ICStdUtil::SPrintF_S(key, 15, "payload/%i", index);
        CStrWChar path(key);
        found = FindStringAttribute(path, CStrWChar(L"name"), &fileName);
        if (found) {
            unsigned int crc = 0;
            CStrWChar url;
            FindStringAttribute(path, CStrWChar(L"uri"), &url);
            CStrWChar crcText;
            FindStringAttribute(path, CStrWChar(L"crc32"), &crcText);
            if (crcText.GetLength() > 0) {
                char crcBuffer[crcText.GetLength() + 1];
                int crcLength = crcText.GetLength();
                if (wcstombs(crcBuffer, crcText.GetWCharPtr(), crcLength + 1) == (size_t)-1) {
                    crcBuffer[0] = 0;
                } else {
                    crcBuffer[crcLength] = 0;
                }
                crc = CGenUtil::HexAToI(crcBuffer);
            }
            CStrWChar dataPath;
            FindStringAttribute(path, CStrWChar(L"path"), &dataPath);
            unsigned long long type;
            FindIntAttribute(path, CStrWChar(L"type"), &type);
            unsigned long long control;
            FindIntAttribute(path, CStrWChar(L"control"), &control);

            CNGSDirectFileDownloadRequest* request =
                new CNGSDirectFileDownloadRequest(fileName, url, crc, dataPath, (eCNGSDirectFileDownloadRequestType)type,
                                                  (eCNGSDirectFileDownloadDownloadControl)control);
            request->m_batchName = m_currentRequest->m_fileName;
            queued = true;
            AddToDownloadQueue(request);
            index++;
        }
    } while (found);

    m_currentRequest->m_batchCount = index;
    if (m_notificationHandler != NULL) {
        m_notificationHandler->HandleBatchFileDownloadStartedMessage(m_currentRequest->m_fileName, index);
    }
    return queued;
}

void CNGSDirectFileDownloadRequestFunctor::requestCompleteCallback(char* data, int length, bool notModified)
{
    if (m_callback) {
        (m_target->*m_callback)(data, length, notModified);
    }
}

void CNGSDirectFileDownloadRequestFunctor::requestCompleteCallback(CObjectMap* response)
{
}
