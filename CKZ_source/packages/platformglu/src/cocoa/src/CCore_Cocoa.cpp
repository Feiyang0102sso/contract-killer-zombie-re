// CCore_Cocoa.cpp - iOS platform core: state, language and orientation handling that needs no UIKit
// (the UIKit part is in CCore_Cocoa_mm.mm).
// the auto-rotation tables are copied with the C library (see notes/b002.md: prefix mapping)
#undef memset
#undef memcpy
#include <string.h>
#include <wchar.h>
#include <mach/mach.h>
#include <CoreFoundation/CoreFoundation.h>

#include "CCore_Cocoa.h"
#include "CStrChar.h"
#include "CStrWChar.h"

using namespace com::glu::platform::components;
using namespace com::glu::platform::core;
using namespace com::glu::platform::framework;

// ---- constants ----
static const unsigned int SystemEvent_AutoRotationChanged = 0x58568b6c;   // queued after the view moved

ICCore* ICCore::CreateInstance()
{
    return new CCore_Cocoa();
}

CCore_Cocoa::CCore_Cocoa()
{
    memset(m_appliedAutoRotationOrientationEnabled, 0, sizeof(m_appliedAutoRotationOrientationEnabled));

    m_maxTouches = 6;
    m_heapBudget = 0x1e951a0;
    m_watchdogMillis = 30000;
    m_hasAccelerometer = 1;
    m_hasTouchScreen = 1;

    // telephony and vibration only exist on the iPhone
    if (wcsncmp(L"iPhone", GetPlatformIdentifier().GetWCharPtr(), 5) == 0) {
        m_hasTelephony = 1;
        m_canVibrate = 1;
    } else {
        m_hasTelephony = 0;
        m_canVibrate = 0;
    }

    m_keyboardType = 3;
    m_autoRotationIsActive = 1;
    m_autoRotationAllowed = 1;
    m_autoRotationOrientationEnabled[AutoRotation_All] = 1;
    memcpy(m_appliedAutoRotationOrientationEnabled, m_autoRotationOrientationEnabled,
           sizeof(m_appliedAutoRotationOrientationEnabled));
}

CCore_Cocoa::~CCore_Cocoa()
{
}

void CCore_Cocoa::ExitApp(eExitStatus status)
{
    CApplet::m_pThis->ExitApp();
}

unsigned char CCore_Cocoa::CanDetectLanguage() const
{
    return 1;
}

unsigned char CCore_Cocoa::DetectLanguage(CStrChar& language) const
{
    language = "";

    CFArrayRef languages = CFLocaleCopyPreferredLanguages();
    if (CFArrayGetCount(languages) > 0) {
        CFStringRef preferred = (CFStringRef)CFArrayGetValueAtIndex(languages, 0);
        language = CFStringGetCStringPtr(preferred, kCFStringEncodingMacRoman);
    } else {
        CFLocaleRef locale = CFLocaleCopyCurrent();
        CFStringRef code = (CFStringRef)CFLocaleGetValue(locale, kCFLocaleLanguageCode);
        language = CFStringGetCStringPtr(code, kCFStringEncodingMacRoman);
        CFRelease(locale);
    }
    CFRelease(languages);
    return 1;
}

unsigned char CCore_Cocoa::ApplicationCanSetOrientation() const
{
    return m_applicationCanSetOrientation;
}

ICCore::eDeviceOrientation CCore_Cocoa::GetDeviceOrientation() const
{
    eDeviceOrientation orientation = DeviceOrientation_Portrait;
    if (m_deviceOrientationSet) {
        orientation = DeviceOrientation_Unknown;
        switch (m_deviceOrientation) {
        case DeviceOrientation_Portrait:
            orientation = DeviceOrientation_Portrait;
            break;
        case DeviceOrientation_LandscapeRight:
            orientation = DeviceOrientation_LandscapeRight;
            break;
        case DeviceOrientation_PortraitUpsideDown:
            orientation = DeviceOrientation_PortraitUpsideDown;
            break;
        case DeviceOrientation_LandscapeLeft:
            orientation = DeviceOrientation_LandscapeLeft;
            break;
        default:
            break;
        }
    }
    return orientation;
}

unsigned char CCore_Cocoa::AutoRotationIsActive() const
{
    return 1;
}

ICCore::eSoftkeyPosition CCore_Cocoa::GetSoftkeyPosition(eKeyCode key) const
{
    if (key == KeyCode_SoftkeyLeft) {
        return SoftkeyPosition_BottomLeft;
    }
    return SoftkeyPosition_BottomRight;
}

unsigned char CCore_Cocoa::CanMultipleTouch() const
{
    return 1;
}

unsigned char CCore_Cocoa::IsMultipleTouchEnabled() const
{
    return CApplet::m_pThis->m_multipleTouchEnabled;
}

void CCore_Cocoa::SetMultipleTouchEnabled(unsigned char enabled)
{
    CApplet::m_pThis->m_multipleTouchEnabled = enabled;
}

unsigned int CCore_Cocoa::GetDeviceAvailableMemory()
{
    mach_msg_type_number_t count;
    vm_size_t pageSize;
    vm_statistics_data_t statistics;

    mach_port_t host = mach_host_self();
    count = HOST_VM_INFO_COUNT;
    host_page_size(host, &pageSize);
    if (host_statistics(host, HOST_VM_INFO, (host_info_t)&statistics, &count) != KERN_SUCCESS) {
        return 0;
    }
    return statistics.free_count * pageSize;
}

void CCore_Cocoa::UpdateAutoRotationOrientation()
{
    const unsigned char* applied = m_appliedAutoRotationOrientationEnabled;
    const unsigned char* wanted = m_autoRotationOrientationEnabled;

    // the view lives in the view controller only while some orientation is enabled
    unsigned char detach = (applied[AutoRotation_Portrait] || applied[AutoRotation_LandscapeRight] ||
                   applied[AutoRotation_PortraitUpsideDown] || applied[AutoRotation_LandscapeLeft]) &&
                  !(wanted[AutoRotation_Portrait] || wanted[AutoRotation_LandscapeRight] ||
                    wanted[AutoRotation_PortraitUpsideDown] || wanted[AutoRotation_LandscapeLeft]);
    unsigned char attach = !(applied[AutoRotation_Portrait] || applied[AutoRotation_LandscapeRight] ||
                    applied[AutoRotation_PortraitUpsideDown] || applied[AutoRotation_LandscapeLeft]) &&
                  (wanted[AutoRotation_Portrait] || wanted[AutoRotation_LandscapeRight] ||
                   wanted[AutoRotation_PortraitUpsideDown] || wanted[AutoRotation_LandscapeLeft]);

    if (detach == 1) {
        DetachView();
    }
    for (int index = 0; index < AutoRotation_Count; index++) {
        m_appliedAutoRotationOrientationEnabled[index] = m_autoRotationOrientationEnabled[index];
    }
    if (attach == 1) {
        AttachView();
    }
    CApplet::m_pThis->QueueSystemEvent(SystemEvent_AutoRotationChanged, 0, 0);
}
