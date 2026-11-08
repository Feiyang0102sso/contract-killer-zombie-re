// PushNotificationGift.h - gifts granted through push notifications.
// Only the static gift list survived in the original binary (the functions using it
// were stripped by the linker), so the Gift layout below is a guess.
#ifndef PUSHNOTIFICATIONGIFT_H
#define PUSHNOTIFICATIONGIFT_H

#include "Vector_ss.h"

class PushNotificationGift {
public:
    /** One pending gift (contents unknown; nothing in the binary reads it). */
    struct Gift {
        int m_type;
        int m_amount;
    };

    static Vector<Gift> m_gifts;
};

#endif
