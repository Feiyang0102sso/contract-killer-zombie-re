// Platform hooks for the offline gServe and save paths.
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include "BundleInterface.h"
#include "CFileUtil_gServe.h"
#include "CNGSFileMgr.h"
#include "CNGS_Platform.h"
#include "CNGSSocialInterface.h"
#include "CPlatformUtil.h"
#include "CPushNotification_Cocoa.h"
#include "ICFileMgr.h"
#include "iosinterface.h"
#include "timelib.h"

using namespace com::glu::platform;
using namespace com::glu::platform::components;

CNGS_Platform::CNGS_Platform()
    : CSingleton(ClassId), m_gameCenterInterface(nullptr), m_facebookInterface(nullptr),
      m_reserved10(0), m_gameCenterSupported(0)
{
}

CNGS_Platform::~CNGS_Platform() = default;
void CNGS_Platform::showPromptMessageDialog(CStrWChar&, CNGSMessagePrompt*, core::CVector<const CStrWChar*>*, unsigned char) {}
void CNGS_Platform::showGenericTextEntryDialog(CStrWChar&, CDialogFunctor* functor, const math::CVector2d&, wchar_t**)
{
    if (functor != nullptr) {
        functor->buttonPressed(0);
        delete functor;
    }
}
void CNGS_Platform::showGenericAlertPopup(const CStrWChar& message)
{
    fwprintf(stderr, L"%ls\n", message.GetWCharPtr());
}

CNGSSocialInterface* CNGS_Platform::getSocialInterface(int type)
{
    if (type == SOCIAL_INTERFACE_FACEBOOK) {
        static CNGSSocialInterface offlineFacebook;
        return &offlineFacebook;
    }
    return nullptr;
}

CStrWChar CPlatformUtil::getDeviceName() { return CStrWChar(L"CKZ PC"); }
void BundleInterface::BundleIdentifierString(CStrWChar& text) { text = L"com.glu.ckzombies"; }
void BundleInterface::BundleVersionString(CStrWChar& text) { text = L"3.1.1"; }

CPushNotification_Cocoa::CPushNotification_Cocoa()
    : CSingleton(SingletonId), m_state(State_Failed), m_context(nullptr), m_callback(nullptr)
{
}
CPushNotification_Cocoa::~CPushNotification_Cocoa() = default;
unsigned char CPushNotification_Cocoa::Register(const wchar_t*, const wchar_t*, void*, Callback) { return 0; }
unsigned char CPushNotification_Cocoa::Cancel() { return 0; }
unsigned char CPushNotification_Cocoa::IsEnabled() { return 0; }
unsigned char CPushNotification_Cocoa::GetIdentity(CStrWChar&) { return 0; }
unsigned char CPushNotification_Cocoa::SetBadgeNumber(unsigned int) { return 0; }
unsigned char CPushNotification_Cocoa::GetBadgeNumber(unsigned int&) { return 0; }

unsigned char CNGSFileMgr::MkDir(const wchar_t* path) { return ICFileMgr::GetInstance()->MkDir(path); }
int CFileUtil_gServe::GetModificationDate(const wchar_t* path)
{
    struct _stat64 info;
    if (_wstat64(path, &info) != 0) {
        return 0;
    }
    return static_cast<int>(info.st_mtime);
}
void CFileUtil_gServe::GetApplicationCachePath(CStrWChar& path)
{
    path = ICFileMgr::GetInstance()->GetApplicationCachePath();
}

void IOSInterface::showMessage(const char* title, const char* message)
{
    fprintf(stderr, "%s: %s\n", title, message);
}

void timelib_time2local(timelib_time* result, timelib_sll seconds)
{
    const auto timestamp = static_cast<__time64_t>(seconds);
    struct tm local = {};
    timelib_time2struc(result, seconds);
    if (_localtime64_s(&local, &timestamp) == 0) {
        result->y = local.tm_year + 1900;
        result->m = local.tm_mon + 1;
        result->d = local.tm_mday;
        result->h = local.tm_hour;
        result->i = local.tm_min;
        result->s = local.tm_sec;
    }
    result->is_localtime = 1;
    result->have_zone = 1;
}
