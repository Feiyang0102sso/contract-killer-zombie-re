// CNGSFileMgr.mm - directory creation through NSFileManager.
#include "CNGSFileMgr.h"

#include "CStrWChar.h"

using namespace com::glu::platform;

unsigned char CNGSFileMgr::MkDir(const wchar_t* path)
{
    components::CStrWChar directoryPath(path);
    char narrowPath[1024] = { 0 };
    wcstombs(narrowPath, directoryPath.m_string, wcslen(directoryPath.m_string) + 1);
    NSString* nsPath = [[[NSString alloc] initWithUTF8String:narrowPath] autorelease];
    BOOL isDirectory = NO;
    BOOL result = YES;
    if (![[NSFileManager defaultManager] fileExistsAtPath:nsPath isDirectory:&isDirectory]) {
        result = [[NSFileManager defaultManager] createDirectoryAtPath:nsPath withIntermediateDirectories:YES
                                                            attributes:nil error:nil];
    }
    return result;
}
