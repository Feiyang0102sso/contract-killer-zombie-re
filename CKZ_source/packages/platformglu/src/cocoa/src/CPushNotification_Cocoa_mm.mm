// CPushNotification_Cocoa_mm.mm - remote notifications: register with Apple (APNs), then hand the
// device token to the provider (a caller supplied callback, or Urban Airship over ASIHTTPRequest).
#include "CPushNotification_Cocoa.h"

#include "ASIHTTPRequest.h"
#include "CApplet.h"
#include "CStdUtil_Cocoa.h"

using namespace com::glu::platform::components;

/** System event posted when a remote notification arrives. */
static const unsigned int kEventPushNotification = 0x5f70b0e3;

NSString* WCharToNSString(const wchar_t* text)
{
    return [[[NSString alloc] initWithBytes:text
                                     length:wcslen(text) * sizeof(wchar_t)
                                   encoding:NSUTF32LittleEndianStringEncoding] autorelease];
}

CPushNotification_Cocoa::CPushNotification_Cocoa()
    : CSingleton(SingletonId)
{
    m_state = State_Idle;
    m_context = NULL;
    m_callback = NULL;
}

CPushNotification_Cocoa::~CPushNotification_Cocoa()
{
}

void CPushNotification_Cocoa::RegisterWithApple()
{
    if (m_state != State_Idle) {
        return;
    }
    m_state = State_RegisteringWithApple;
    [[UIApplication sharedApplication] registerForRemoteNotificationTypes:
        (UIRemoteNotificationType)(UIRemoteNotificationTypeBadge | UIRemoteNotificationTypeSound |
                                   UIRemoteNotificationTypeAlert)];
}

unsigned char CPushNotification_Cocoa::Register(const wchar_t* appKey, const wchar_t* appSecret,
                                                void* context, Callback callback)
{
    if (m_state != State_Idle) {
        return 0;
    }
    m_appKey = appKey;
    m_appSecret = appSecret;
    m_context = context;
    m_callback = callback;
    RegisterWithApple();
    return 1;
}

unsigned char CPushNotification_Cocoa::Cancel()
{
    [[UIApplication sharedApplication] unregisterForRemoteNotifications];
    return 1;
}

unsigned char CPushNotification_Cocoa::IsEnabled()
{
    return m_state == State_Registered && m_deviceToken.GetLength() > 0;
}

unsigned char CPushNotification_Cocoa::GetIdentity(CStrWChar& identity)
{
    if (!IsEnabled()) {
        return 0;
    }
    identity = m_deviceToken;
    return 1;
}

void CPushNotification_Cocoa::RegisterWithAppleCallback(unsigned char success, const char* deviceToken)
{
    if (m_state != State_RegisteringWithApple) {
        return;
    }
    if (success == 1) {
        m_deviceToken = deviceToken;
        RegisterWithProvider();
    } else {
        m_state = State_Failed;
    }
}

void CPushNotification_Cocoa::RegisterWithProvider()
{
    if (m_state != State_RegisteringWithApple) {
        return;
    }

    if (m_callback != NULL) {
        // Custom provider: hand it a copy of the device token
        CStrWChar deviceToken(m_deviceToken);
        if (m_callback(m_context, &deviceToken) == 1) {
            m_state = State_Registered;
        } else {
            m_state = State_Failed;
        }
    } else {
        RegisterWithProviderUrbanAirship();
    }
}

void CPushNotification_Cocoa::NotificationCallback(unsigned char launchedApplication, const wchar_t* message)
{
    m_message = message;
    // Low 4 bits: how the notification arrived, the rest: time stamp
    unsigned int timeAndFlags = (CStdUtil_Cocoa::GetUpTimeMS() << 4) | (launchedApplication & 0xf);
    CApplet::m_pThis->QueueSystemEvent(kEventPushNotification, (unsigned int)&m_message, timeAndFlags);
}

void CPushNotification_Cocoa::RegisterWithProviderUrbanAirship()
{
    NSOperationQueue* queue = [[[NSOperationQueue alloc] init] autorelease];

    // PUT https://go.urbanairship.com/api/device_tokens/<token>/
    NSString* deviceToken = WCharToNSString(m_deviceToken.GetWCharPtr());
    NSString* urlString = [NSString stringWithFormat:@"%@%@%@/", @"https://go.urbanairship.com",
                           @"/api/device_tokens/", deviceToken];
    NSURL* url = [NSURL URLWithString:urlString];
    ASIHTTPRequest* request = [[[ASIHTTPRequest alloc] initWithURL:url] autorelease];
    [request setRequestMethod:@"PUT"];

    // Optional device alias
    NSString* alias = [[NSUserDefaults standardUserDefaults] stringForKey:@"_UADeviceAliasKey"];
    if (alias != nil && [alias length] != 0) {
        [request addRequestHeader:@"Content-Type" value:@"application/json"];
        NSString* body = [NSString stringWithFormat:@"{\"alias\": \"%@\"}", alias];
        [request appendPostData:[body dataUsingEncoding:NSUTF8StringEncoding]];
    }

    // HTTP basic authentication with the application key / secret
    [request setUsername:WCharToNSString(m_appKey.GetWCharPtr())];
    [request setPassword:WCharToNSString(m_appSecret.GetWCharPtr())];
    [queue addOperation:request];
    m_state = State_Registered;
}

unsigned char CPushNotification_Cocoa::SetBadgeNumber(unsigned int badgeNumber)
{
    [[UIApplication sharedApplication] setApplicationIconBadgeNumber:badgeNumber];
    return 1;
}

unsigned char CPushNotification_Cocoa::GetBadgeNumber(unsigned int& badgeNumber)
{
    badgeNumber = [[UIApplication sharedApplication] applicationIconBadgeNumber];
    return 1;
}
