// CSaveManager.cpp - CRC-protected XML saves, XOR payload and temporary-file replacement.
#include "CSaveManager.h"
#include "ICFileMgr.h"
#include "ICFile.h"
#include "CCrc32.h"
#include "CBH_RSACrypt.h"
#include "DataOutputStream.h"
#include "DataInputStream.h"
#include "WindowBase.h"
#include "IOSInterface.h"

using namespace com::glu::platform::components;

// Named original data: 11 and 15 UTF-32 code units, including their terminators.
extern const wchar_t SAVE_FILE_NAME[] = L"savebh.dat";
static const wchar_t TMP_FILE_NAME[] = L"savebh_tmp.dat";

CSaveManager::CSaveManager()
{
    m_flags = 0;
    m_saveState = -1;
}

ESaveResult CSaveManager::Init()
{
    m_pending = false;
    m_force = 0;
    m_pauseGServe = 1;
    XString profile("default");
    XString profileCopy(profile);
    CStrWChar path(ICFileMgr::GetInstance()->GetApplicationDataPath());
    path = ICFileMgr::GetInstance()->GetApplicationDataPath();
    if (path.GetLength() > 0) {
        path += ICFileMgr::GetInstance()->GetPathSeparator() + CStrWChar(XString::AnsiString(profileCopy.c_str()));
    }
    // The original adds the filename only when this directory already exists.
    if (ICFileMgr::GetInstance()->ExistsDir(path.GetWCharPtr())) {
        path += ICFileMgr::GetInstance()->GetPathSeparator() + CStrWChar(SAVE_FILE_NAME);
    }
    m_fileName.Assign(path.GetWCharPtr(), wcslen(path.GetWCharPtr()) * sizeof(wchar_t));
    ICFile* file = ICFileMgr::GetInstance()->Open(path.GetWCharPtr(), ICFileMgr::AccessRead);
    if (file != NULL) {
        unsigned int size = ICFileMgr::GetInstance()->Size(path.GetWCharPtr());
        unsigned char* data = new unsigned char[size];
        int read = file->Read(data, size);
        ParseData(data, read, &m_document);
        ICFileMgr::GetInstance()->Close(file);
        delete[] data;
        m_root = m_document.FirstChildElement();
    } else {
        TiXmlElement* root = new TiXmlElement("savefile");
        m_root = m_document.InsertEndChild(*root)->ToElement();
        delete root;
    }
    // Parse failures are reported by ParseData but not propagated by Init.
    return SAVE_SUCCESS;
}

void CSaveManager::PauseByGServe(unsigned char paused)
{
    m_pauseGServe = paused;
}

ESaveResult CSaveManager::ParseData(unsigned char* data, int size, TiXmlDocument* document)
{
    unsigned int expected = (unsigned int)data[0] << 24 | (unsigned int)data[1] << 16 |
                            (unsigned int)data[2] << 8 | data[3];
    unsigned int actual = CCrc32::GetInstance()->Crc32(data + 4, size - 4);
    if (actual != expected) {
        ShowSaveErrorMessage(SAVE_CRC_ERROR);
        return SAVE_CRC_ERROR;
    }
    CBH_XorCrypt::Decypher((char*)data + 5, size - 5, data[4]);
    document->Clear();
    document->Parse((char*)data + 5);
    // Original 0x1312ea reads Document+0x2c; the real inline Error keeps the same bool semantics.
    if (!document->Error()) {
        return SAVE_SUCCESS;
    }
    ShowSaveErrorMessage(SAVE_XML_ERROR);
    return SAVE_XML_ERROR;
}

unsigned int CSaveManager::GetData(unsigned char** data)
{
    TiXmlPrinter printer;
    m_document.Accept(&printer);
    // Original 0x131890/0x131892 read the Rep at Printer+0x0c and its length; the characters start at Rep+8.
    const char* serialized = printer.CStr();
    int size = printer.Size();
    unsigned char* payload = new unsigned char[size];
    for (int index = 0; index < size; ++index) {
        payload[index] = serialized[index];
    }
    unsigned char key = CBH_XorCrypt::Cipher((char*)payload, size);
    DataOutputStream output;
    DataOutputStream body;
    body.writeByte(key);
    body.write(payload, size);
    unsigned int crc = CCrc32::GetInstance()->Crc32(body.GetData(), body.m_buffer.size());
    unsigned char* crcBytes = new unsigned char[4];
    crcBytes[0] = crc >> 24;
    crcBytes[1] = crc >> 16;
    crcBytes[2] = crc >> 8;
    crcBytes[3] = crc;
    output.write(crcBytes, 4);
    delete[] crcBytes;
    output.write(body.GetData(), body.m_buffer.size());
    unsigned char* bytes = output.GetData();
    unsigned char* result = new unsigned char[(unsigned int)output.m_buffer.size()];
    for (unsigned int index = 0; index < (unsigned int)output.m_buffer.size(); ++index) {
        result[index] = bytes[index];
    }
    *data = result;
    delete[] payload;
    return output.m_buffer.size();
}

ESaveResult CSaveManager::Save(bool force)
{
    // The original ignores force and writes whenever saving is not paused.
    if (m_pauseSaving || m_pauseGServe != 0) {
        return SAVE_PAUSED;
    }
    XString profile("default");
    CStrWChar path(ICFileMgr::GetInstance()->GetApplicationDataPath());
    if (path.GetLength() > 0) {
        path += CStrWChar(ICFileMgr::GetInstance()->GetPathSeparator()) + CStrWChar(profile.c_str());
    }
    if (!ICFileMgr::GetInstance()->ExistsDir(path.GetWCharPtr())) {
        ICFileMgr::GetInstance()->MkDir(path.GetWCharPtr());
    }
    path += ICFileMgr::GetInstance()->GetPathSeparator() + CStrWChar(TMP_FILE_NAME);
    ICFile* file = ICFileMgr::GetInstance()->Open(path.GetWCharPtr(), ICFileMgr::AccessWrite);
    DataOutputStream unusedStream; // Constructed in the original despite being unused.
    unsigned char* buffer = NULL;
    if (file != NULL) {
        unsigned int size = GetData(&buffer);
        if (file->Write(buffer, size) != size) {
            ICFileMgr::GetInstance()->Close(file);
            delete[] buffer;
            ShowSaveErrorMessage(SAVE_WRITE_ERROR);
            return SAVE_WRITE_ERROR;
        }
    }
    ICFileMgr::GetInstance()->Close(file);
    bool valid = IsSaveValid(path);
    ICFileMgr* fileManager = ICFileMgr::GetInstance();
    if (valid == true) {
        {
            CStrWChar finalPath(fileManager->GetApplicationDataPath());
            if (finalPath.GetLength() > 0) {
                finalPath += ICFileMgr::GetInstance()->GetPathSeparator() + CStrWChar(XString::AnsiString(profile.c_str()));
            }
            ICFileMgr::GetInstance()->ExistsDir(finalPath.GetWCharPtr());
            finalPath += ICFileMgr::GetInstance()->GetPathSeparator() + CStrWChar(SAVE_FILE_NAME);
            ICFileMgr::GetInstance()->Delete(finalPath.GetWCharPtr());
            ICFileMgr::GetInstance()->Rename(path.GetWCharPtr(), finalPath.GetWCharPtr());
        }
        m_saveState = 1;
        delete[] buffer;
        return SAVE_SUCCESS;
    }
    // Original validation-failure branch leaves buffer allocated.
    fileManager->Delete(path.GetWCharPtr());
    ShowSaveErrorMessage(SAVE_WRITE_ERROR);
    return SAVE_WRITE_ERROR;
}

bool CSaveManager::IsSaveValid(const CStrWChar& path)
{
    ICFile* file = ICFileMgr::GetInstance()->Open(path.GetWCharPtr(), ICFileMgr::AccessRead);
    ICFileMgr* fileManager = ICFileMgr::GetInstance();
    if (file != NULL) {
        unsigned int size = fileManager->Size(path.GetWCharPtr());
        unsigned char* buffer = new unsigned char[size];
        file->Read(buffer, size);
        JDataInputStream stream((char*)buffer, size);
        unsigned int expected = stream.readInt();
        unsigned int actual = CCrc32::GetInstance()->Crc32(buffer + 4, size - 4);
        ICFileMgr::GetInstance()->Close(file);
        delete[] buffer;
        return actual == expected;
    }
    fileManager->Close(file);
    return false;
}

void CSaveManager::ShowSaveErrorMessage(ESaveResult result)
{
    XString title = Window::ResString("IDS_SAVE_ERROR_TITLE");
    title = XString::Format(title.c_str(), result);
    XString text = Window::ResString("IDS_SAVE_ERROR_TEXT");
    IOSInterface::showMessage(XString::AnsiString(title.c_str()), XString::AnsiString(text.c_str()));
}

TiXmlElement* CSaveManager::GetRootElement()
{
    return m_root;
}

void CSaveManager::Update()
{
    if (m_pending) {
        Save(m_force != 0);
        m_pending = false;
        m_force = 0;
    }
}

void CSaveManager::SaveOnNextUpdate(unsigned char force)
{
    m_pending = true;
    m_force = force;
}

void CSaveManager::Reset()
{
    ICFileMgr::GetInstance()->Delete(m_fileName.c_str());
    m_document.Clear();
    TiXmlElement* root = new TiXmlElement("savefile");
    m_root = m_document.InsertEndChild(*root)->ToElement();
    delete root;
}

void CSaveManager::PauseSaving()
{
    Save(false);
    m_pauseSaving = true;
}

void CSaveManager::ResumeSaving()
{
    m_pauseSaving = false;
    Save(false);
}
