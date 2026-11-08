// CPHManager.h - Glu's Objective-C front end to the PlayHaven SDK (created by b033x).
#ifndef CPHMANAGER_H
#define CPHMANAGER_H

#import "PlayHavenSDK.h"

/** Optional C++ observer of content requests (registerListener:). No virtual destructor: slot 0 is OnRequestWillGetContent. */
class CPHListener {
public:
    virtual void OnRequestWillGetContent() = 0;
    virtual void OnContentWillDisplay() = 0;
    virtual void OnContentDidDisplay() = 0;
    virtual void OnContentDidDismiss() = 0;
    /** `error` is the UTF-8 localized description. */
    virtual void OnRequestFailed(const char* error) = 0;
};

@interface CPHManager : NSObject <PHPublisherContentRequestDelegate> {
    NSMutableDictionary* m_notificationViews;   // placement name -> PHNotificationView
    NSString* m_activePlacement;                // placement of the content request in flight
    NSString* m_token;
    NSString* m_secret;
    CPHListener* m_listener;
    bool m_showErrorMessage;                    // show an alert when the active request fails
}

@property (nonatomic, retain) NSString* activePlacement;

- (id)init:(const char*)token :(const char*)secret;
- (void)startPublisherOpenRequest;
- (void)startPublisherContentRequest:(const char*)placement :(bool)showsOverlayImmediately :(bool)showErrorMessage;
- (void)cancelPublisherContentRequest;
- (bool)isPublisherContentRequestActive:(const char*)placement;
- (void)showErrorMessage:(int)errorCode;
- (void)showNotificationView:(const char*)placement :(bool)testMode :(unsigned int)centerX :(unsigned int)centerY;
- (void)hideNotificationView:(const char*)placement;
- (void)registerListener:(CPHListener*)listener;

@end

#endif
