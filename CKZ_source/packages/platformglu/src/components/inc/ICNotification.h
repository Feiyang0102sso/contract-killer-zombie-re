// ICNotification.h - local (scheduled) and push (remote) notifications of the platform layer.
#ifndef ICNOTIFICATION_H
#define ICNOTIFICATION_H

#include <wchar.h>

namespace com { namespace glu { namespace platform { namespace components {

class CStrWChar;

class ICNotification {
public:
    /** Kind of notification to register (CGameApp registers push with 1). */
    enum Type {
        Type_Local = 0,
        Type_Push = 1
    };

    /**
     * Registration hook: called with the caller context and the platform object
     * (push: CStrWChar* device token, local: UILocalNotification*). Returns 1 on success.
     */
    typedef unsigned char (*Callback)(void* context, void* data);

    /** Platform factory (CNotification_Cocoa). */
    static ICNotification* CreateInstance();

    virtual ~ICNotification() {}

    /** Push: register with the provider using its application key / secret. */
    virtual unsigned char Register(Type type, const wchar_t* appKey, const wchar_t* appSecret,
                                   void* context, Callback callback) = 0;
    /** Local: schedule `message` in `delaySeconds` with the given application badge. */
    virtual unsigned char Register(Type type, unsigned int delaySeconds, unsigned int badgeNumber,
                                   const wchar_t* message, const wchar_t* action,
                                   void* context, Callback callback) = 0;
    virtual unsigned char Cancel() = 0;
    virtual unsigned char IsEnabled() = 0;
    /** Push device token. */
    virtual unsigned char GetIdentity(CStrWChar& identity) = 0;
    virtual unsigned char SetBadgeNumber(unsigned int badgeNumber) = 0;
    virtual unsigned char GetBadgeNumber(unsigned int& badgeNumber) = 0;
};

} } } }

#endif
