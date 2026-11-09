// Win32 wide paths let players install the port in a non-ASCII directory.
#include "Host.h"
#include <filesystem>
#include "CFile_Cocoa.h"
#include "ICFileMgr.h"

using namespace com::glu::platform::components;
namespace fs = std::filesystem;

namespace {

class FileMgrSDL final : public ICFileMgr {
public:
    const wchar_t* GetApplicationPath() override { return ckz::assetPath; }
    const wchar_t* GetApplicationDataPath() override { return ckz::savePath; }
    const wchar_t* GetApplicationTempPath() override { return ckz::savePath; }
    const wchar_t* GetApplicationCachePath() override { return ckz::savePath; }
    const wchar_t* GetPathSeparator() override { return L"/"; }

    ICFile* Open(const wchar_t* path, eFileAccessMode mode) override
    {
        if (path == nullptr) {
            return nullptr;
        }
        const wchar_t* modes[] = { L"rb", L"wb", L"ab" };
        FILE* stream = _wfopen(path, modes[mode]);
        if (stream == nullptr) {
            return nullptr;
        }
        auto* file = new CFile_Cocoa;
        file->m_file = stream;
        return file;
    }

    void Close(ICFile* file) override { delete file; }

    unsigned char Exists(const wchar_t* path) override
    {
        return path != nullptr && fs::is_regular_file(path, m_error);
    }

    unsigned int Size(const wchar_t* path) override
    {
        if (path == nullptr) {
            return 0;
        }
        const auto size = fs::file_size(path, m_error);
        if (m_error) {
            return 0;
        }
        return static_cast<unsigned int>(size);
    }

    unsigned char Rename(const wchar_t* from, const wchar_t* to) override
    {
        if (from == nullptr || to == nullptr) {
            return 0;
        }
        fs::rename(from, to, m_error);
        return !m_error;
    }

    unsigned char Delete(const wchar_t* path) override
    {
        return path != nullptr && fs::remove(path, m_error);
    }

    unsigned char ExistsDir(const wchar_t* path) override
    {
        return path != nullptr && fs::is_directory(path, m_error);
    }

    unsigned char MkDir(const wchar_t* path) override
    {
        if (path == nullptr) {
            return 0;
        }
        fs::create_directories(path, m_error);
        return !m_error;
    }

    unsigned char RmDir(const wchar_t* path) override { return Delete(path); }

    unsigned char DeleteFilesInDir(const wchar_t* path) override
    {
        if (!EnumInit(path, 0)) {
            return 0;
        }
        EnumDirectoryFileInfo entry;
        while (EnumNext(&entry)) {
            if (!Delete((fs::path(path) / entry.m_name).c_str())) {
                return 0;
            }
        }
        return 1;
    }

    unsigned int GetFreeDiskSpace() override
    {
        const auto available = fs::space(ckz::savePath, m_error).available;
        if (m_error) {
            return 0;
        }
        if (available > UINT_MAX) {
            return UINT_MAX;
        }
        return static_cast<unsigned int>(available);
    }

    unsigned char EnumInit(const wchar_t* path, unsigned char directories) override
    {
        if (path == nullptr || *path == 0) {
            path = ckz::savePath;
        }
        m_enum = fs::directory_iterator(path, m_error);
        m_directories = directories != 0;
        return !m_error;
    }

    unsigned char EnumNext(EnumDirectoryFileInfo* info) override
    {
        const fs::directory_iterator end;
        while (m_enum != end) {
            const fs::directory_entry entry = *m_enum;
            m_enum.increment(m_error);
            if (entry.is_directory(m_error) != m_directories) {
                continue;
            }
            if (info != nullptr) {
                m_name = entry.path().filename().wstring();
                info->m_name = m_name.c_str();
                info->m_size = 0;
                info->m_attributes = 0;
                if (m_directories) {
                    info->m_attributes = EnumDirectoryFileInfo::Attribute_Directory;
                } else {
                    info->m_size = Size(entry.path().c_str());
                }
            }
            return 1;
        }
        return 0;
    }

private:
    fs::directory_iterator m_enum;
    std::wstring m_name;
    std::error_code m_error;
    bool m_directories = false;
};

} // namespace

ICFileMgr* ICFileMgr::CreateInstance() { return new FileMgrSDL; }
