// CAppInfo_Cocoa_mm.mm - iOS manifest of the application: bundle identifier and the
// "major.minor.micro" CFBundleVersion from Info.plist.
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "CAppInfo.h"

using namespace com::glu::platform::components;

void CAppInfo::CacheManifestInfo()
{
    m_bundleIdentifier = [[[NSBundle mainBundle] bundleIdentifier] UTF8String];

    CStrWChar version([[[[NSBundle mainBundle] infoDictionary] objectForKey:(NSString*)kCFBundleVersionKey] UTF8String]);
    char text[256];
    wcstombs(text, version.GetWCharPtr(), wcslen(version.GetWCharPtr()) + 1);

    // split "major.minor.micro" in place
    char* minor = strchr(text, '.');
    *minor++ = 0;
    char* micro = strchr(minor, '.');
    *micro++ = 0;

    m_versionMajor = atoi(text);
    m_versionMinor = atoi(minor);
    m_versionMicro = atoi(micro);
}
