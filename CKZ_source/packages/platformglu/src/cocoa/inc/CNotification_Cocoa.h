// CNotification_Cocoa.h - iOS notifications: forwards to the push singleton or the local notification.
#ifndef CNOTIFICATION_COCOA_H
#define CNOTIFICATION_COCOA_H

#include "ICNotification.h"

class CPushNotification_Cocoa;
class CLocalNotification_Cocoa;

class CNotification_Cocoa : public com::glu::platform::components::ICNotification {
public:
    CNotification_Cocoa()
    {
        m_push = NULL;
        m_local = NULL;
    }

    virtual ~CNotification_Cocoa();

    virtual unsigned char Register(Type type, const wchar_t* appKey, const wchar_t* appSecret,
                                   void* context, Callback callback);
    virtual unsigned char Register(Type type, unsigned int delaySeconds, unsigned int badgeNumber,
                                   const wchar_t* message, const wchar_t* action,
                                   void* context, Callback callback);
    virtual unsigned char Cancel();
    virtual unsigned char IsEnabled();
    virtual unsigned char GetIdentity(com::glu::platform::components::CStrWChar& identity);
    virtual unsigned char SetBadgeNumber(unsigned int badgeNumber);
    virtual unsigned char GetBadgeNumber(unsigned int& badgeNumber);

private:
    CPushNotification_Cocoa* m_push;        // 0x04 application singleton, not owned
    CLocalNotification_Cocoa* m_local;      // 0x08 owned
};

#endif
