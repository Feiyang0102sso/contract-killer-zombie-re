// LocalPushManager.h - schedules / cancels iOS local push notifications (UILocalNotification).
#ifndef LOCALPUSHMANAGER_H
#define LOCALPUSHMANAGER_H

/**
 * Thin static facade over UILocalNotification.
 * All strings are UTF-8; times are seconds from now.
 */
class LocalPushManager {
public:
    /** Cancels every scheduled local notification; optionally clears the app icon badge. */
    static void CancelLocalPushNotifications(bool clearBadge);

    /** Schedules one notification firing `delaySeconds` from now. */
    static void SetupLocalPush(const char* alertBody, const char* alertAction, int badgeNumber, int delaySeconds);

    /** Public entry point: same as SetupLocalPush with the last two arguments swapped. */
    static void SetLocalPushNotification(const char* alertBody, const char* alertAction, int delaySeconds, int badgeNumber);
};

#endif // LOCALPUSHMANAGER_H
