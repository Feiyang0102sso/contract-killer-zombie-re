// CLocalNotification_Cocoa.h - one scheduled UILocalNotification (iOS 4+).
#ifndef CLOCALNOTIFICATION_COCOA_H
#define CLOCALNOTIFICATION_COCOA_H

#include <wchar.h>

#ifdef __OBJC__
@class UILocalNotification;
#else
typedef void UILocalNotification;
#endif

namespace com { namespace glu { namespace platform { namespace components {
class CStrWChar;
} } } }

class CLocalNotification_Cocoa {
public:
    /** Customization hook: (context, UILocalNotification*) replaces the default scheduling. */
    typedef unsigned char (*Callback)(void* context, void* data);

    CLocalNotification_Cocoa();
    virtual ~CLocalNotification_Cocoa();

    /** Schedule `message` in `delaySeconds`; fails when one is already scheduled or on iOS < 4. */
    virtual unsigned char Register(unsigned int delaySeconds, unsigned int badgeNumber,
                                   const wchar_t* message, const wchar_t* action,
                                   void* context, Callback callback);
    virtual unsigned char Cancel();
    virtual unsigned char IsEnabled();
    virtual unsigned char GetIdentity(com::glu::platform::components::CStrWChar& identity);
    virtual unsigned char SetBadgeNumber(unsigned int badgeNumber);
    virtual unsigned char GetBadgeNumber(unsigned int& badgeNumber);

    /** A local notification fired (posted as a system event). */
    static void NotificationCallback(unsigned char launchedApplication, const void* userInfo);

private:
    UILocalNotification* m_notification;    // 0x04 retained while scheduled
};

#endif
