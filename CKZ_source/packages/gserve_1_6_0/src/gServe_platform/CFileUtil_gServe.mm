// CFileUtil_gServe.mm - the Foundation-backed parts of CFileUtil_gServe (file dates, cache folder).
#include "CFileUtil_gServe.h"

#include <string.h>
#include "CUtf.h"

using namespace com::glu::platform;

/** The OS cache folder, filled by the first GetApplicationCachePath() call. */
wchar_t m_wcsApplicationCachePath[256];

int CFileUtil_gServe::GetModificationDate(const wchar_t* path)
{
    int modificationDate = 0;
    char narrowPath[1024] = { 0 };
    wcstombs(narrowPath, path, wcslen(path) + 1);
    NSString* filePath = [NSString stringWithUTF8String:narrowPath];
    BOOL isDirectory = NO;
    if ([[NSFileManager defaultManager] fileExistsAtPath:filePath isDirectory:&isDirectory]) {
        NSError* error;
        NSDictionary* attributes = [[NSFileManager defaultManager] attributesOfItemAtPath:filePath error:&error];
        NSDate* date = [attributes objectForKey:NSFileModificationDate];
        modificationDate = (int)[date timeIntervalSince1970];
    }
    return modificationDate;
}

/** First caches directory of the user domain; *outPath is left alone when there is none. */
void getCacheDirectory(NSString** outPath)
{
    NSArray* paths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
    if ([paths count] != 0) {
        *outPath = [paths objectAtIndex:0];
    }
}

void CFileUtil_gServe::GetApplicationCachePath(components::CStrWChar& outPath)
{
    if (m_wcsApplicationCachePath[0] == 0) {
        NSString* cachePath = @"";
        getCacheDirectory(&cachePath);
        const char* utf8Path = [cachePath UTF8String];
        if (utf8Path != NULL) {
            core::CUtf::Utf8ToWcs((const unsigned char*)utf8Path, strlen(utf8Path), m_wcsApplicationCachePath, 256);
        }
    }
    outPath = m_wcsApplicationCachePath;
}
