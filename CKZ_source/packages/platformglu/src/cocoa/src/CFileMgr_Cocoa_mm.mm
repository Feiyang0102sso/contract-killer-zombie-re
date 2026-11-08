// CFileMgr_Cocoa_mm.mm - Foundation part of the iOS file system: standard paths and directory
// enumeration (see CFileMgr_Cocoa.h).
#include <string.h>
#include "CFileMgr_Cocoa.h"
#include "CUtf.h"

using namespace com::glu::platform::components;
using namespace com::glu::platform::core;

unsigned char CFileMgr_Cocoa::EnumInit(const wchar_t* path, unsigned char directories)
{
    m_enumDirectories = directories;

    NSString* directory;
    if (path[0] == 0) {
        // empty path: the application data directory
        CUtf::WcsToUtf8(GetApplicationDataPath(), wcslen(GetApplicationDataPath()),
                        (unsigned char*)m_utf8Path, PathLength);
        directory = [NSString stringWithUTF8String:m_utf8Path];
    } else {
        CUtf::WcsToUtf8(path, wcslen(path), (unsigned char*)m_utf8Path, PathLength);
        directory = [NSString stringWithUTF8String:m_utf8Path];
    }
    if (directory == nil) {
        return 0;
    }

    m_enumerator = [[NSFileManager defaultManager] enumeratorAtPath:directory];
    return m_enumerator != nil;
}

unsigned char CFileMgr_Cocoa::EnumNext(EnumDirectoryFileInfo* info)
{
    if (m_enumerator == nil) {
        return 0;
    }

    // skip entries of the other kind (files / directories); never descend
    NSString* name;
    NSDictionary* attributes;
    id fileType;
    do {
        name = [m_enumerator nextObject];
        if (name == nil) {
            return 0;
        }
        [m_enumerator skipDescendents];
        attributes = [m_enumerator fileAttributes];
        fileType = [attributes objectForKey:NSFileType];
    } while ((m_enumDirectories != 0) != (fileType == NSFileTypeDirectory));

    if (info == NULL) {
        return 1;
    }

    const char* utf8Name = [name UTF8String];
    if (utf8Name != NULL) {
        CUtf::Utf8ToWcs((const unsigned char*)utf8Name, strlen(utf8Name), m_enumFileName, PathLength);
    }
    info->m_name = m_enumFileName;
    info->m_size = [[attributes objectForKey:NSFileSize] longValue];
    info->m_attributes = 0;
    if (fileType == NSFileTypeDirectory) {
        info->m_attributes = EnumDirectoryFileInfo::Attribute_Directory;
    }
    return 1;
}

void CFileMgr_Cocoa::ReadPathInfo()
{
    // application: the bundle resources
    m_applicationPath[0] = 0;
    NSBundle* bundle = [NSBundle mainBundle];
    if (bundle != nil) {
        NSString* resourcePath = [bundle resourcePath];
        if (resourcePath != nil) {
            const char* text = [resourcePath UTF8String];
            if (text != NULL) {
                CUtf::Utf8ToWcs((const unsigned char*)text, strlen(text), m_applicationPath, PathLength);
            }
        }
    }

    // data: Documents
    m_dataPath[0] = 0;
    NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    if ([paths count] != 0) {
        NSString* first = [paths objectAtIndex:0];
        if (first != nil) {
            const char* text = [first UTF8String];
            if (text != NULL) {
                CUtf::Utf8ToWcs((const unsigned char*)text, strlen(text), m_dataPath, PathLength);
            }
        }
    }

    // temporary files: Caches
    m_tempPath[0] = 0;
    paths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
    if ([paths count] != 0) {
        NSString* first = [paths objectAtIndex:0];
        if (first != nil) {
            const char* text = [first UTF8String];
            if (text != NULL) {
                CUtf::Utf8ToWcs((const unsigned char*)text, strlen(text), m_tempPath, PathLength);
            }
        }
    }

    // cache: Caches
    m_cachePath[0] = 0;
    paths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
    if ([paths count] != 0) {
        NSString* first = [paths objectAtIndex:0];
        if (first != nil) {
            const char* text = [first UTF8String];
            if (text != NULL) {
                CUtf::Utf8ToWcs((const unsigned char*)text, strlen(text), m_cachePath, PathLength);
            }
        }
    }
}
