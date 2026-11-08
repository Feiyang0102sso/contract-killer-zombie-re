// CLocalNotification_Cocoa_mm.mm - one UILocalNotification scheduled through UIApplication (iOS 4+).
#include "CLocalNotification_Cocoa.h"

#include "CApplet.h"
#include "CPushNotification_Cocoa.h"
#include "CStdUtil_Cocoa.h"

/** System event posted when a local notification fires. */
static const unsigned int kEventLocalNotification = 0x9a2ea6eb;

CLocalNotification_Cocoa::CLocalNotification_Cocoa()
    : m_notification(nil)
{
}

CLocalNotification_Cocoa::~CLocalNotification_Cocoa()
{
    if (m_notification != nil &&
        [[UIApplication sharedApplication] respondsToSelector:@selector(scheduledLocalNotifications)]) {
        [m_notification release];
        m_notification = nil;
    }
}

unsigned char CLocalNotification_Cocoa::Register(unsigned int delaySeconds, unsigned int badgeNumber,
                                                 const wchar_t* message, const wchar_t* action,
                                                 void* context, Callback callback)
{
    if (m_notification != nil) {
        return 0;
    }
    // Local notifications need iOS 4
    if (![[UIApplication sharedApplication] respondsToSelector:@selector(scheduledLocalNotifications)]) {
        return 0;
    }

    UILocalNotification* notification = [[UILocalNotification alloc] init];
    if (callback != NULL) {
        // The caller configures and schedules the notification itself
        callback(context, notification);
    } else {
        [notification setFireDate:[[NSDate date] dateByAddingTimeInterval:delaySeconds]];
        [notification setTimeZone:[NSTimeZone defaultTimeZone]];
        if (message != NULL) {
            [notification setAlertBody:WCharToNSString(message)];
        }
        if (action != NULL) {
            [notification setAlertAction:WCharToNSString(action)];
        }
        [notification setSoundName:UILocalNotificationDefaultSoundName];
        [notification setApplicationIconBadgeNumber:badgeNumber];
        [[UIApplication sharedApplication] scheduleLocalNotification:notification];
    }
    m_notification = notification;
    return 1;
}

unsigned char CLocalNotification_Cocoa::Cancel()
{
    unsigned char result = 0;

    if (m_notification != nil &&
        [[UIApplication sharedApplication] respondsToSelector:@selector(scheduledLocalNotifications)]) {
        UILocalNotification* notification = m_notification;
        [[UIApplication sharedApplication] cancelLocalNotification:notification];
        [notification release];
        m_notification = nil;
        result = 1;
    }
    return result;
}

unsigned char CLocalNotification_Cocoa::IsEnabled()
{
    return m_notification != nil;
}

unsigned char CLocalNotification_Cocoa::GetIdentity(com::glu::platform::components::CStrWChar& identity)
{
    return 0;
}

unsigned char CLocalNotification_Cocoa::SetBadgeNumber(unsigned int badgeNumber)
{
    if (m_notification == nil) {
        return 0;
    }
    [[UIApplication sharedApplication] setApplicationIconBadgeNumber:badgeNumber];
    return 1;
}

unsigned char CLocalNotification_Cocoa::GetBadgeNumber(unsigned int& badgeNumber)
{
    if (m_notification == nil) {
        return 0;
    }
    badgeNumber = [[UIApplication sharedApplication] applicationIconBadgeNumber];
    return 1;
}

void CLocalNotification_Cocoa::NotificationCallback(unsigned char launchedApplication, const void* userInfo)
{
    // Low 4 bits: how the notification arrived, the rest: time stamp
    unsigned int timeAndFlags = (CStdUtil_Cocoa::GetUpTimeMS() << 4) | (launchedApplication & 0xf);
    CApplet::m_pThis->QueueSystemEvent(kEventLocalNotification, 0, timeAndFlags);
}
