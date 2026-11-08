// CFacebookManager.h - Objective-C side of the Facebook interface: session (login / logout),
// Graph API requests (user info, friends, pictures) and the feed dialog, reporting back to
// CFacebookInterface through CNGS_Platform. Ivar layout from the original ObjC metadata.
// Owned by social/ios/CFacebookManager.mm (b027, written by b027x).
#ifndef CFACEBOOKMANAGER_H
#define CFACEBOOKMANAGER_H

#import <Foundation/Foundation.h>
#import "FBConnect.h"

class CNGSSocialMessage;

@class CFacebookPostMessageDelegate;
@class CFacebookUserInfoDelegate;
@class CFacebookLoadFriendsDelegate;

/** Request kinds reported by the delegates (didLoad:/didFailWithError: requestType:). */
enum {
    FB_REQUEST_USER_INFO = 0,
    FB_REQUEST_LOAD_FRIENDS = 1,
    FB_REQUEST_POST_MESSAGE = 2,
    FB_REQUEST_USER_PIC = 3
};

/** m_connectionState / m_postMessageState values. */
enum {
    FB_STATE_IDLE = 0,
    FB_STATE_PENDING = 1,
    FB_STATE_DONE = 2,
    FB_STATE_FAILED = 3,
    FB_STATE_CANCELLED = 4
};

@interface CFacebookManager : NSObject <FBRequestDelegate, FBSessionDelegate> {
    Facebook* m_facebook;                                   // +0x04
    CFacebookPostMessageDelegate* m_fbPostMsgDelegate;      // +0x08
    CFacebookUserInfoDelegate* m_fbUserInfoDelegate;        // +0x0c
    CFacebookLoadFriendsDelegate* m_fbLoadFriendsDelegate;  // +0x10
    NSArray* m_permissions;                                 // +0x14
    int m_request;                                          // +0x18 last FB_REQUEST_* sent
    int m_connectionState;                                  // +0x1c FB_STATE_*
    int m_postMessageState;                                 // +0x20 FB_STATE_*
    NSMutableDictionary* m_userInfo;                        // +0x24 uid / name / email
    NSMutableArray* m_friendList;                           // +0x28 friend ids
    NSMutableDictionary* m_friendInfo;                      // +0x2c id -> { name, picture }
    NSString* m_kAppId;                                     // +0x30
    BOOL m_loginAutomatically;                              // +0x34 the saved token was still valid
    NSMutableArray* m_fbUserPicDelegates;                   // +0x38 picture requests in flight
}

- (id)init;
- (void)dealloc;
- (void)setAppID:(NSString*)appId;
- (void)login;
- (void)logout;
- (void)clearData;
- (void)getUserInfo;
- (void)loadFriends;
- (void)fbDidLogin;
- (void)fbDidNotLogin:(BOOL)cancelled;
- (void)fbDidLogout;
- (void)request:(FBRequest*)request didReceiveResponse:(NSURLResponse*)response;
- (void)didFailWithError:(NSError*)error requestType:(int)requestType uid:(NSString*)uid;
- (void)didLoad:(id)result requestType:(int)requestType uid:(NSString*)uid;
- (void)handleLoadPostMessageResponse:(id)result;
- (void)handleGetUserInfoResponse:(id)result;
- (void)handleLoadFriendsResponse:(id)result;
- (void)handleLoadUserPicResponse:(id)result uid:(NSString*)uid;
- (int)getConnectionState;
- (int)getPostConnectionState;
- (NSString*)getUserID;
- (NSString*)getUserName;
- (NSString*)getUserEmail;
- (int)getFriendsCount;
- (NSMutableArray*)getFriends;
- (NSMutableDictionary*)getFriendsInfo;
/** Open the feed dialog with the parameters of `message`. */
- (void)publishStream:(CNGSSocialMessage*)message;
- (BOOL)handleOpenURL:(NSURL*)url;
- (void)dialogDidComplete:(FBDialog*)dialog;
- (void)dialogCompleteWithUrl:(NSURL*)url;
- (void)dialogDidNotCompleteWithUrl:(NSURL*)url;
- (void)dialogDidNotComplete:(FBDialog*)dialog;
- (void)dialog:(FBDialog*)dialog didFailWithError:(NSError*)error;
- (BOOL)dialog:(FBDialog*)dialog shouldOpenURLInExternalBrowser:(NSURL*)url;
/** Request the picture of `uid` (a friend or the player); NO when too many are in flight or unknown. */
- (BOOL)loadUserPic:(NSString*)uid type:(NSString*)type;
- (void)removeUserPicDelegate:(NSString*)uid;

@end

/** Base of the request / dialog delegates: forwards results to the manager. */
@interface CFacebookDelegate : NSObject <FBRequestDelegate, FBDialogDelegate> {
    CFacebookManager* m_FBManager;      // +0x04 not retained
}

- (id)init;
- (void)dealloc;
- (void)setFBManager:(CFacebookManager*)manager;

@end

/** Feed dialog result. */
@interface CFacebookPostMessageDelegate : CFacebookDelegate
@end

/** "me" request result. */
@interface CFacebookUserInfoDelegate : CFacebookDelegate
@end

/** Picture request of one user. */
@interface CFacebookUserPicDelegate : CFacebookDelegate {
    NSString* m_uid;                    // +0x08 owned copy
}

- (void)setUid:(NSString*)uid;
- (NSString*)getUid;

@end

/** "me/friends" request result. */
@interface CFacebookLoadFriendsDelegate : CFacebookDelegate
@end

#endif
