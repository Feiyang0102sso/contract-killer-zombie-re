// LocalPushManager_Cocoa.mm - iOS implementation of LocalPushManager.
#include "LocalPushManager.h"

#import <UIKit/UIKit.h>

void LocalPushManager::CancelLocalPushNotifications(bool clearBadge)
{
    // cancelAllLocalNotifications only exists on iOS 4+
    if ([[UIApplication sharedApplication] respondsToSelector:@selector(cancelAllLocalNotifications)]) {
        if (clearBadge) {
            [[UIApplication sharedApplication] setApplicationIconBadgeNumber:0];
        }
        [[UIApplication sharedApplication] cancelAllLocalNotifications];
    }
}

void LocalPushManager::SetupLocalPush(const char* alertBody, const char* alertAction, int badgeNumber, int delaySeconds)
{
    UILocalNotification* notification = [[UILocalNotification alloc] init];
    if (notification == nil) {
        return;
    }

    [notification setFireDate:[[NSDate date] dateByAddingTimeInterval:delaySeconds]];
    [notification setTimeZone:[NSTimeZone defaultTimeZone]];
    if (alertBody != NULL) {
        [notification setAlertBody:[NSString stringWithUTF8String:alertBody]];
    }
    if (alertAction != NULL) {
        [notification setAlertAction:[NSString stringWithUTF8String:alertAction]];
    }
    [notification setSoundName:UILocalNotificationDefaultSoundName];
    [notification setApplicationIconBadgeNumber:badgeNumber];
    [[UIApplication sharedApplication] scheduleLocalNotification:notification];
    [notification release];
}

void LocalPushManager::SetLocalPushNotification(const char* alertBody, const char* alertAction, int delaySeconds, int badgeNumber)
{
    SetupLocalPush(alertBody, alertAction, badgeNumber, delaySeconds);
}
