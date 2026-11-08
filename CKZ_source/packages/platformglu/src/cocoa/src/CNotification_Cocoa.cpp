// CNotification_Cocoa.cpp - routes ICNotification calls to the push singleton (when registered)
// or to the owned local notification.
#include "CNotification_Cocoa.h"

#include "CPushNotification_Cocoa.h"
#include "CLocalNotification_Cocoa.h"

using namespace com::glu::platform::components;

ICNotification* ICNotification::CreateInstance()
{
    return new CNotification_Cocoa();
}

CNotification_Cocoa::~CNotification_Cocoa()
{
    if (m_local != NULL) {
        delete m_local;
        m_local = NULL;
    }
}

unsigned char CNotification_Cocoa::Register(Type type, const wchar_t* appKey, const wchar_t* appSecret,
                                            void* context, Callback callback)
{
    unsigned char result = 0;
    m_push = CPushNotification_Cocoa::GetInstance();
    if (m_push != NULL) {
        result = m_push->Register(appKey, appSecret, context, callback);
    }
    return result;
}

unsigned char CNotification_Cocoa::Register(Type type, unsigned int delaySeconds, unsigned int badgeNumber,
                                            const wchar_t* message, const wchar_t* action,
                                            void* context, Callback callback)
{
    if (m_local == NULL) {
        m_local = new CLocalNotification_Cocoa();
        if (m_local == NULL) {
            return 0;
        }
    }
    return m_local->Register(delaySeconds, badgeNumber, message, action, context, callback);
}

unsigned char CNotification_Cocoa::Cancel()
{
    unsigned char result = 0;

    if (m_push != NULL) {
        result = m_push->Cancel();
        m_push = NULL;
    }
    if (m_local != NULL) {
        result = m_local->Cancel();
        if (m_local != NULL) {
            delete m_local;
            m_local = NULL;
        }
    }
    return result;
}

unsigned char CNotification_Cocoa::IsEnabled()
{
    if (m_push != NULL) {
        return m_push->IsEnabled();
    }
    if (m_local != NULL) {
        return m_local->IsEnabled();
    }
    return 0;
}

unsigned char CNotification_Cocoa::GetIdentity(CStrWChar& identity)
{
    if (m_push != NULL) {
        return m_push->GetIdentity(identity);
    }
    if (m_local != NULL) {
        return m_local->GetIdentity(identity);
    }
    return 0;
}

unsigned char CNotification_Cocoa::SetBadgeNumber(unsigned int badgeNumber)
{
    if (m_push != NULL) {
        return m_push->SetBadgeNumber(badgeNumber);
    }
    if (m_local != NULL) {
        return m_local->SetBadgeNumber(badgeNumber);
    }
    return 0;
}

unsigned char CNotification_Cocoa::GetBadgeNumber(unsigned int& badgeNumber)
{
    if (m_push != NULL) {
        return m_push->GetBadgeNumber(badgeNumber);
    }
    if (m_local != NULL) {
        return m_local->GetBadgeNumber(badgeNumber);
    }
    return 0;
}
