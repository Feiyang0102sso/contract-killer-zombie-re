// CUpdateManager.cpp - downloads, records, and installs cached resource updates.
#include "CUpdateManager.h"
#include "CNGSDirectFileDownload.h"
#include "CNGSDirectFileDownloadRequest.h"
#include "ICFileMgr.h"
#include "ICFile.h"
#include "CAppInfo.h"
#include "CFileUtil.h"
#include "CFileUtil_gServe.h"
#include "xString.h"

using namespace com::glu::platform::components;

CUpdateManager::CUpdateManager()
    : m_onInstalled(NULL)
{
    m_downloadFailed = false;
    m_newUpdatesInstalled = false;
}

CUpdateManager::~CUpdateManager()
{
    CNGSDirectFileDownload* download = CNGSDirectFileDownload::GetInstance();
    if (download != NULL) {
        download->registerNotificationHandler(NULL);
    }
}

void CUpdateManager::init()
{
    makeWorkPath(m_workPath);
    if (m_workPath.GetLength() == 0) {
        return;
    }
    makeUpdatesListPath(m_updatesPath);
    makeCompleteUpdatesListPath(m_completePath);
    ICFileMgr::GetInstance()->MkDir(m_workPath.GetWCharPtr());
    CAppInfo* info = CAppInfo::GetInstance();
    int major = info->GetVersionMajor();
    int minor = info->GetVersionMinor();
    int micro = info->GetVersionMicro();
    CStrWChar folder(XString::Format(L"%s_%d_%d_%d", L"ver", major, minor, micro).c_str());
    CNGSDirectFileDownload::GetInstance()->SetFolder(folder);
}

void CUpdateManager::checkUpdates()
{
    if (m_workPath.GetLength() == 0 || m_updatesPath.GetLength() == 0 || m_completePath.GetLength() == 0) {
        return;
    }
    if (CNGSDirectFileDownload::GetInstance()->GetQueued().Size() != 0) {
        return;
    }
    ICFileMgr* files = ICFileMgr::GetInstance();
    if (files->Exists(m_completePath.GetWCharPtr()) != 0) {
        return;
    }
    m_downloadFailed = false;
    CNGSDirectFileDownload* download = CNGSDirectFileDownload::GetInstance();
    unsigned char partial = files->Exists(m_updatesPath.GetWCharPtr());
    // The original selects the download control after constructing the string arguments.
    download->AddToDownloadQueue(new CNGSDirectFileDownloadRequest(
        CStrWChar(L"batch"), CStrWChar(L"batch.json"), 0, CStrWChar(L""),
        DIRECT_FILE_DOWNLOAD_BATCH, partial != 0 ? DOWNLOAD_CONTROL_ALWAYS : DOWNLOAD_CONTROL_IF_MODIFIED));
    download->registerNotificationHandler(this);
    download->StartNextDownload();
}

int CUpdateManager::hasCompleteUpdates() const
{
    if (m_workPath.GetLength() == 0 || m_updatesPath.GetLength() == 0 || m_completePath.GetLength() == 0) {
        return 0;
    }
    return ICFileMgr::GetInstance()->Exists(m_completePath.GetWCharPtr()) != 0;
}

int CUpdateManager::installUpdates(bool silent)
{
    if (m_workPath.GetLength() == 0 || m_updatesPath.GetLength() == 0 || m_completePath.GetLength() == 0) {
        return 0;
    }
    ICFileMgr* files = ICFileMgr::GetInstance();
    if (files->Exists(m_completePath.GetWCharPtr()) == 0) {
        return 0;
    }
    ICFile* list = files->Open(m_completePath.GetWCharPtr(), ICFileMgr::AccessRead);
    if (list == NULL) {
        return 0;
    }
    wchar_t* fileName = (wchar_t*)malloc(4096);
    const wchar_t* folder = CNGSDirectFileDownload::GetInstance()->GetFolder();
    int installed = 0;
    unsigned int length;
    if (list->Read((unsigned char*)&length, 4) == 4) {
        installed = 1;
        do {
            list->Read((unsigned char*)fileName, length);
            fileName[length / sizeof(wchar_t)] = 0;
            copyFromCacheToWork(fileName, folder);
            if (!silent) {
                m_newUpdatesInstalled = true;
            }
        } while (list->Read((unsigned char*)&length, 4) == 4);
    }
    if (fileName != NULL) {
        free(fileName);
    }
    files->Close(list);
    files->Delete(m_completePath.GetWCharPtr());
    bool skipCallback = installed != 1;
    if (installed == 1) {
        skipCallback = silent;
    }
    if (!skipCallback && m_onInstalled != NULL) {
        m_onInstalled();
    }
    return installed;
}

void CUpdateManager::copyFromCacheToWork(const wchar_t* fileName, const wchar_t* folder) const
{
    CStrWChar cachedPath;
    CFileUtil_gServe::GenerateCacheFilePath(folder, fileName, cachedPath);
    if (cachedPath.GetLength() != 0) {
        CStrWChar workPath;
        workPath += m_workPath;
        workPath += L"/";
        workPath += fileName;
        copyFile(cachedPath.GetWCharPtr(), workPath.GetWCharPtr());
    }
}

void CUpdateManager::update(unsigned int elapsed)
{
    CNGSDirectFileDownload::GetInstance()->HandleUpdate(elapsed);
}

unsigned char CUpdateManager::HandleFileDownloadedMessage(unsigned char success, CNGSDirectFileDownloadRequest& request)
{
    if (request.m_type == DIRECT_FILE_DOWNLOAD_DATA) {
        fileComplete(request);
        if (CNGSDirectFileDownload::GetInstance()->GetQueued().Size() == 0) {
            batchComplete();
        }
    }
    return 1;
}

unsigned char CUpdateManager::HandleBatchFileDownloadStartedMessage(const CStrWChar& batch, int count)
{
    if (CNGSDirectFileDownload::GetInstance()->GetQueued().Size() == 0) {
        batchComplete();
    }
    return 0;
}

unsigned char CUpdateManager::HandleBatchFileDownloadCompleteMessage(const CStrWChar& batch, int count)
{
    batchComplete();
    return 1;
}

void CUpdateManager::fileComplete(const CNGSDirectFileDownloadRequest& request)
{
    if (m_workPath.GetLength() == 0 || m_updatesPath.GetLength() == 0 || m_completePath.GetLength() == 0
        || request.m_type != DIRECT_FILE_DOWNLOAD_DATA) {
        return;
    }
    if (request.m_status == DOWNLOAD_STATUS_COMPLETE) {
        ICFileMgr* files = ICFileMgr::GetInstance();
        ICFile* list = ICFileMgr::GetInstance()->Open(m_updatesPath.GetWCharPtr(), ICFileMgr::AccessAppend);
        if (list != NULL) {
            if (request.m_fileName.GetLength() < 1024) {
                int bytes = request.m_fileName.GetLength() * sizeof(wchar_t);
                list->Write((const unsigned char*)&bytes, 4);
                list->Write((const unsigned char*)request.m_fileName.GetWCharPtr(), bytes);
                ICFileMgr::GetInstance()->Close(list);
            } else {
                files->Close(list);
            }
        }
    } else if (request.m_status == DOWNLOAD_STATUS_CRC_ERROR || request.m_status == DOWNLOAD_STATUS_EMPTY) {
        m_downloadFailed = true;
    }
}

void CUpdateManager::batchComplete()
{
    if (m_workPath.GetLength() == 0 || m_updatesPath.GetLength() == 0 || m_completePath.GetLength() == 0) {
        return;
    }
    int queued = CNGSDirectFileDownload::GetInstance()->GetQueued().Size();
    if (queued != 0 || m_downloadFailed) {
        return;
    }
    ICFileMgr* files = ICFileMgr::GetInstance();
    if (files->Exists(m_updatesPath.GetWCharPtr()) != 0) {
        if (files->Exists(m_completePath.GetWCharPtr()) != 0) {
            files->Delete(m_completePath.GetWCharPtr());
        }
        files->Rename(m_updatesPath.GetWCharPtr(), m_completePath.GetWCharPtr());
    }
}

void CUpdateManager::makeWorkPath(CStrWChar& path) const
{
    CAppInfo* info = CAppInfo::GetInstance();
    int major = info->GetVersionMajor();
    int minor = info->GetVersionMinor();
    int micro = info->GetVersionMicro();
    CFileUtil::GetApplicationCachePathForFile(path,
        XString::Format(L"%s-%d.%d.%d", L"resources", major, minor, micro).c_str());
}

void CUpdateManager::makeResourcePath(const wchar_t* fileName, CStrWChar& path) const
{
    path = m_workPath;
    path += L"/";
    path += fileName;
}

void CUpdateManager::makeUpdatesListPath(CStrWChar& path) const
{
    makeResourcePath(L".updates", path);
}

void CUpdateManager::makeCompleteUpdatesListPath(CStrWChar& path) const
{
    makeResourcePath(L".complete_updates", path);
}

int CUpdateManager::copyFile(const wchar_t* source, const wchar_t* destination)
{
    if (source == NULL || destination == NULL) {
        return 0;
    }
    ICFile* input = ICFileMgr::GetInstance()->Open(source, ICFileMgr::AccessRead);
    if (input == NULL) {
        return 0;
    }
    ICFile* output = ICFileMgr::GetInstance()->Open(destination, ICFileMgr::AccessWrite);
    if (output == NULL) {
        ICFileMgr::GetInstance()->Close(input);
        return 0;
    }
    unsigned char* buffer = (unsigned char*)malloc(4096);
    unsigned int count;
    do {
        count = input->Read(buffer, 4096);
        if (count == 0) {
            break;
        }
        output->Write(buffer, count); // Original deliberately ignores short writes.
    } while (count == 4096);
    ICFileMgr::GetInstance()->Close(output);
    ICFileMgr::GetInstance()->Close(input);
    if (buffer != NULL) {
        free(buffer);
    }
    return 1;
}
