// CFacebookManager.mm - Facebook SDK glue of the gServe social interface.
// The manager keeps the session (access token in NSUserDefaults), the player info, the friend
// list and the friend pictures; results go back to the Facebook CNGSSocialInterface through
// CNGS_Platform::getSocialInterface(SOCIAL_INTERFACE_FACEBOOK).
// Request results arrive through small delegate objects (one per request kind) that tag them with
// the request type (FB_REQUEST_*).
// Note: the assert in -login must stay on line 83 (its line number is compiled into the code).
#include "CFacebookManager.h"
#include <assert.h>
#include "AppDelegate.h"
#include "CFacebookInterface.h"
#include "CNGS.h"
#include "CNGS_Platform.h"
#include "CNGSLocalUser.h"
#include "CNGSSocialInterface.h"
#include "CNGSSocialMessage.h"
#include "CNotificationHandler.h"
#include "CUtf.h"
#include "CUtil.h"

using com::glu::platform::components::CStrWChar;
using com::glu::platform::core::CUtf;
using com::glu::platform::core::CVector;

/** At most this many picture requests are in flight. */
static const unsigned int MAX_USER_PIC_REQUESTS = 10;

@implementation CFacebookManager

- (id)init
{
    self = [super init];
    if (self != nil) {
        m_userInfo = [[NSMutableDictionary alloc] init];
        m_permissions = [[NSArray arrayWithObjects:@"read_stream", @"offline_access", @"email", @"publish_stream", nil] retain];
        m_loginAutomatically = YES;
        m_friendList = [[NSMutableArray alloc] init];
        m_friendInfo = [[NSMutableDictionary alloc] init];
        m_fbUserPicDelegates = [[NSMutableArray alloc] init];
        m_facebook = nil;
        m_fbPostMsgDelegate = [[CFacebookPostMessageDelegate alloc] init];
        [m_fbPostMsgDelegate setFBManager:self];
        m_fbUserInfoDelegate = [[CFacebookUserInfoDelegate alloc] init];
        [m_fbUserInfoDelegate setFBManager:self];
        m_fbLoadFriendsDelegate = [[CFacebookLoadFriendsDelegate alloc] init];
        [m_fbLoadFriendsDelegate setFBManager:self];
        m_postMessageState = FB_STATE_IDLE;
    }
    return self;
}

- (void)dealloc
{
    [m_permissions release];
    [m_facebook release];
    [m_userInfo release];
    [m_kAppId release];
    [m_fbUserPicDelegates release];
    [m_friendList release];
    [m_friendInfo release];
    [super dealloc];
}

- (void)setAppID:(NSString*)appId
{
    // (sic) the old id is only "released" when there is none
    if (m_kAppId == nil) {
        [m_kAppId release];
    }
    m_kAppId = appId;
    [m_kAppId retain];
    if (m_facebook == nil) {
        m_facebook = [[[[Facebook alloc] initWithAppId:m_kAppId andDelegate:self] autorelease] retain];
    }
}

/**
 * Restore the saved session when there is one, otherwise open the Facebook login page.
 * A valid session goes straight to the user info request.
 */
- (void)login
{
    assert(m_kAppId);
    m_connectionState = FB_STATE_PENDING;
    if (![m_facebook isSessionValid]) {
        NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
        NSString* accessToken = [defaults objectForKey:@"fb_access_token"];
        [m_facebook setAccessToken:accessToken];
        NSDate* expirationDate = [defaults objectForKey:@"fb_expiration_date"];
        [m_facebook setExpirationDate:expirationDate];
    }
    if ([m_facebook isSessionValid]) {
        m_loginAutomatically = YES;
        [self getUserInfo];
    } else {
        m_loginAutomatically = NO;
        [m_facebook authorize:m_permissions];
    }
}

- (void)logout
{
    m_connectionState = FB_STATE_PENDING;
    [m_facebook logout:self];
}

- (void)clearData
{
    NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
    [defaults removeObjectForKey:@"fb_access_token"];
    [defaults removeObjectForKey:@"fb_expiration_date"];
    [m_userInfo removeAllObjects];
    [m_friendList removeAllObjects];
    [m_friendInfo removeAllObjects];
}

- (void)getUserInfo
{
    [m_userInfo removeAllObjects];
    m_connectionState = FB_STATE_PENDING;
    m_request = FB_REQUEST_USER_INFO;
    [m_facebook requestWithGraphPath:@"me?fields=id,name,email" andDelegate:m_fbUserInfoDelegate];
}

- (void)loadFriends
{
    m_connectionState = FB_STATE_PENDING;
    m_request = FB_REQUEST_LOAD_FRIENDS;
    [m_friendList removeAllObjects];
    [m_friendInfo removeAllObjects];
    [m_facebook requestWithGraphPath:@"me/friends?fields=id,name,email" andDelegate:m_fbLoadFriendsDelegate];
}

- (void)fbDidLogin
{
    m_connectionState = FB_STATE_DONE;
    NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
    [defaults setObject:[m_facebook accessToken] forKey:@"fb_access_token"];
    [defaults setObject:[m_facebook expirationDate] forKey:@"fb_expiration_date"];
    [defaults synchronize];
    [self getUserInfo];
}

- (void)fbDidNotLogin:(BOOL)cancelled
{
    m_connectionState = cancelled ? FB_STATE_CANCELLED : FB_STATE_FAILED;
    CNGS_Platform::GetInstance()->getSocialInterface(SOCIAL_INTERFACE_FACEBOOK)->finishLogin();
}

- (void)fbDidLogout
{
    m_connectionState = FB_STATE_DONE;
    [self clearData];
    CNGS_Platform::GetInstance()->getSocialInterface(SOCIAL_INTERFACE_FACEBOOK)->finishLogout();
}

- (void)request:(FBRequest*)request didReceiveResponse:(NSURLResponse*)response
{
    m_connectionState = FB_STATE_DONE;
}

- (void)didFailWithError:(NSError*)error requestType:(int)requestType uid:(NSString*)uid
{
    if (requestType != FB_REQUEST_POST_MESSAGE) {
        m_connectionState = FB_STATE_FAILED;
        switch (requestType) {
        case FB_REQUEST_USER_INFO:
            [self clearData];
            CNGS_Platform::GetInstance()->getSocialInterface(SOCIAL_INTERFACE_FACEBOOK)->finishLogin();
            break;
        case FB_REQUEST_LOAD_FRIENDS:
            CNGS_Platform::GetInstance()->getSocialInterface(SOCIAL_INTERFACE_FACEBOOK)->finishLoadFriends();
            break;
        case FB_REQUEST_USER_PIC:
            [self removeUserPicDelegate:uid];
            break;
        }
    } else {
        m_postMessageState = FB_STATE_FAILED;
        CNGS_Platform::GetInstance()->getSocialInterface(SOCIAL_INTERFACE_FACEBOOK)->finishPostMessage(0);
    }
}

- (void)didLoad:(id)result requestType:(int)requestType uid:(NSString*)uid
{
    if (requestType != FB_REQUEST_POST_MESSAGE && requestType != FB_REQUEST_USER_PIC) {
        m_connectionState = FB_STATE_DONE;
    }
    switch (requestType) {
    case FB_REQUEST_USER_INFO:
        [self handleGetUserInfoResponse:result];
        break;
    case FB_REQUEST_LOAD_FRIENDS:
        [self handleLoadFriendsResponse:result];
        break;
    case FB_REQUEST_USER_PIC:
        [self handleLoadUserPicResponse:result uid:uid];
        break;
    case FB_REQUEST_POST_MESSAGE:
        [self handleLoadPostMessageResponse:result];
        break;
    }
}

- (void)handleLoadPostMessageResponse:(id)result
{
    if (result != nil && [result objectForKey:@"id"] != nil) {
        m_postMessageState = FB_STATE_DONE;
        CNGS_Platform::GetInstance()->getSocialInterface(SOCIAL_INTERFACE_FACEBOOK)->finishPostMessage(1);
    } else {
        m_postMessageState = FB_STATE_FAILED;
        CNGS_Platform::GetInstance()->getSocialInterface(SOCIAL_INTERFACE_FACEBOOK)->finishPostMessage(0);
    }
}

- (void)handleGetUserInfoResponse:(id)result
{
    NSString* userId = [result objectForKey:@"id"];
    NSString* name = [result objectForKey:@"name"];
    NSString* email = [result objectForKey:@"email"];
    [m_userInfo setObject:userId forKey:@"uid"];
    [m_userInfo setObject:name forKey:@"name"];
    if (email == nil) {
        [m_userInfo setObject:@"" forKey:@"email"];
    } else {
        [m_userInfo setObject:email forKey:@"email"];
    }
    CNGS_Platform::GetInstance()->getSocialInterface(SOCIAL_INTERFACE_FACEBOOK)->finishLogin();
}

- (void)handleLoadFriendsResponse:(id)result
{
    [m_friendList removeAllObjects];
    [m_friendInfo removeAllObjects];

    NSArray* friends = [result objectForKey:@"data"];
    for (NSDictionary* friendData in friends) {
        if ([friendData objectForKey:@"id"] != nil) {
            NSString* friendId = [NSString stringWithString:[friendData objectForKey:@"id"]];
            if (![m_friendList containsObject:friendId]) {
                [m_friendList addObject:friendId];
                NSMutableDictionary* info = [[[NSMutableDictionary alloc] init] autorelease];
                NSString* name = nil;
                if ([friendData objectForKey:@"name"] != nil) {
                    name = [NSString stringWithString:[friendData objectForKey:@"name"]];
                }
                [info setObject:name forKey:@"name"];
                [m_friendInfo setObject:info forKey:friendId];
            }
        }
    }

    // rebuild the friend list of the interface, sorted by name
    CNGSSocialInterface* socialInterface = CNGS_Platform::GetInstance()->getSocialInterface(SOCIAL_INTERFACE_FACEBOOK);
    CVector<FriendInfo>* friendsInfos = socialInterface->getSocialNetworkFriendsInfos();
    friendsInfos->Clear();
    for (unsigned int index = 0; index < [m_friendList count]; index++) {
        NSString* friendId = [m_friendList objectAtIndex:index];
        NSString* name = [[m_friendInfo objectForKey:friendId] objectForKey:@"name"];
        if (name == nil) {
            name = @"";
        }
        int position;
        for (position = 0; position < friendsInfos->Size(); position++) {
            NSString* otherName = [CUtil WcharToNSString:(*friendsInfos)[position].m_name.m_string];
            if ([name caseInsensitiveCompare:otherName] == NSOrderedAscending) {
                break;
            }
        }
        FriendInfo info;
        info.m_name = [CUtil NSStringToWchar:name];
        info.m_id = [CUtil NSStringToWchar:friendId];
        friendsInfos->Insert(position, info);
    }
    CNGS_Platform::GetInstance()->getSocialInterface(SOCIAL_INTERFACE_FACEBOOK)->finishLoadFriends();
}

- (void)handleLoadUserPicResponse:(id)result uid:(NSString*)uid
{
    unsigned char success = 0;
    if ([result isKindOfClass:[NSData class]] && uid != nil) {
        NSMutableDictionary* info = [m_friendInfo objectForKey:uid];
        if (info != nil) {
            [info objectForKey:@"picture"];
            NSData* picture = [NSData dataWithData:result];
            if (picture != nil) {
                success = 1;
                [info setObject:picture forKey:@"picture"];
            }
        }
    }
    [self removeUserPicDelegate:uid];
    CNGS_Platform::GetInstance()->getSocialInterface(SOCIAL_INTERFACE_FACEBOOK)->finishLoadFriendPic(success);
}

- (int)getConnectionState
{
    return m_connectionState;
}

- (int)getPostConnectionState
{
    return m_postMessageState;
}

- (NSString*)getUserID
{
    return [m_userInfo objectForKey:@"uid"];
}

- (NSString*)getUserName
{
    return [m_userInfo objectForKey:@"name"];
}

- (NSString*)getUserEmail
{
    return [m_userInfo objectForKey:@"email"];
}

- (int)getFriendsCount
{
    return [m_friendList count];
}

- (NSMutableArray*)getFriends
{
    return m_friendList;
}

- (NSMutableDictionary*)getFriendsInfo
{
    return m_friendInfo;
}

- (void)publishStream:(CNGSSocialMessage*)message
{
    if (m_postMessageState != FB_STATE_PENDING) {
        if ([m_facebook isSessionValid]) {
            m_postMessageState = FB_STATE_PENDING;
            CStrWChar targetId = message->GetTargetUserId();
            int utf8Capacity = 256;
            unsigned char utf8TargetId[utf8Capacity];
            int utf8Length = CUtf::WcsToUtf8(targetId.m_string, message->GetTargetUserId().GetLength(), utf8TargetId, utf8Capacity);
            utf8TargetId[utf8Length] = 0;
            if (targetId.GetLength() > 0) {
                message->SetParam("to", targetId.m_string);
            }
            [m_facebook dialog:@"feed" andParams:(NSMutableDictionary*)message->GetMessage() andDelegate:m_fbPostMsgDelegate];
        }
    }
}

- (BOOL)handleOpenURL:(NSURL*)url
{
    CNGSLocalUser* localUser = CNGS::GetInstance()->GetLocalUser();
    if (localUser->m_notificationHandler != NULL) {
        if (!localUser->m_notificationHandler->HandleFacebookLoginPageClose()) {
            return NO;
        }
    }
    return [m_facebook handleOpenURL:url];
}

- (void)dialogDidComplete:(FBDialog*)dialog
{
}

- (void)dialogCompleteWithUrl:(NSURL*)url
{
}

- (void)dialogDidNotCompleteWithUrl:(NSURL*)url
{
}

- (void)dialogDidNotComplete:(FBDialog*)dialog
{
}

- (void)dialog:(FBDialog*)dialog didFailWithError:(NSError*)error
{
}

- (BOOL)dialog:(FBDialog*)dialog shouldOpenURLInExternalBrowser:(NSURL*)url
{
    return NO;
}

- (BOOL)loadUserPic:(NSString*)uid type:(NSString*)type
{
    if ([m_fbUserPicDelegates count] >= MAX_USER_PIC_REQUESTS) {
        return NO;
    }
    for (unsigned int index = 0; index < [m_fbUserPicDelegates count]; index++) {
        if ([[[m_fbUserPicDelegates objectAtIndex:index] getUid] compare:uid] == NSOrderedSame) {
            // already requested
            return YES;
        }
    }
    NSMutableDictionary* info = [m_friendInfo objectForKey:uid];
    if (info == nil && [[self getUserID] compare:uid] == NSOrderedSame) {
        // the player's own picture
        info = [[[NSMutableDictionary alloc] init] autorelease];
        [m_friendInfo setObject:info forKey:uid];
    }
    if (info == nil) {
        return NO;
    }
    CFacebookUserPicDelegate* delegate = [[CFacebookUserPicDelegate alloc] init];
    [delegate setFBManager:self];
    [delegate setUid:uid];
    NSString* graphPath = [NSString stringWithFormat:@"%@/picture?type=%@", uid, type];
    [m_facebook requestWithGraphPath:graphPath andDelegate:delegate];
    [m_fbUserPicDelegates addObject:delegate];
    return YES;
}

- (void)removeUserPicDelegate:(NSString*)uid
{
    for (unsigned int index = 0; index < [m_fbUserPicDelegates count]; index++) {
        CFacebookUserPicDelegate* delegate = [m_fbUserPicDelegates objectAtIndex:index];
        if ([[delegate getUid] compare:uid] == NSOrderedSame) {
            [m_fbUserPicDelegates removeObject:delegate];
            [delegate release];
            return;
        }
    }
}

@end

@implementation CFacebookDelegate

- (id)init
{
    self = [super init];
    m_FBManager = nil;
    return self;
}

- (void)dealloc
{
    [super dealloc];
}

- (void)setFBManager:(CFacebookManager*)manager
{
    m_FBManager = manager;
}

@end

@implementation CFacebookPostMessageDelegate

- (void)request:(FBRequest*)request didLoad:(id)result
{
}

- (void)request:(FBRequest*)request didFailWithError:(NSError*)error
{
}

/** The feed dialog redirects to ...?post_id=<id>: report the id as the result. */
- (void)dialogCompleteWithUrl:(NSURL*)url
{
    NSMutableDictionary* result = [[[NSMutableDictionary alloc] init] autorelease];
    NSArray* parameters = [[url query] componentsSeparatedByString:@"&"];
    if (parameters != nil) {
        for (NSString* parameter in parameters) {
            NSArray* pair = [parameter componentsSeparatedByString:@"="];
            if (pair != nil && [pair count] == 2) {
                NSString* key = [pair objectAtIndex:0];
                NSString* value = [pair objectAtIndex:1];
                if ([key compare:@"post_id"] == NSOrderedSame) {
                    [result setObject:value forKey:@"id"];
                    break;
                }
            }
        }
    }
    [m_FBManager didLoad:result requestType:FB_REQUEST_POST_MESSAGE uid:nil];
}

- (void)dialogDidNotCompleteWithUrl:(NSURL*)url
{
    [m_FBManager didFailWithError:nil requestType:FB_REQUEST_POST_MESSAGE uid:nil];
}

- (void)dialog:(FBDialog*)dialog didFailWithError:(NSError*)error
{
    [m_FBManager didFailWithError:error requestType:FB_REQUEST_POST_MESSAGE uid:nil];
}

- (BOOL)dialog:(FBDialog*)dialog shouldOpenURLInExternalBrowser:(NSURL*)url
{
    return NO;
}

@end

@implementation CFacebookUserInfoDelegate

- (id)init
{
    self = [super init];
    return self;
}

- (void)dealloc
{
    [super dealloc];
}

- (void)request:(FBRequest*)request didLoad:(id)result
{
    [m_FBManager didLoad:result requestType:FB_REQUEST_USER_INFO uid:nil];
}

- (void)request:(FBRequest*)request didFailWithError:(NSError*)error
{
    [m_FBManager didFailWithError:error requestType:FB_REQUEST_USER_INFO uid:nil];
}

@end

@implementation CFacebookUserPicDelegate

- (id)init
{
    self = [super init];
    m_uid = nil;
    return self;
}

- (void)dealloc
{
    [m_uid release];
    [super dealloc];
}

- (void)request:(FBRequest*)request didLoad:(id)result
{
    [m_FBManager didLoad:result requestType:FB_REQUEST_USER_PIC uid:m_uid];
}

- (void)request:(FBRequest*)request didFailWithError:(NSError*)error
{
    [m_FBManager didFailWithError:error requestType:FB_REQUEST_USER_PIC uid:m_uid];
}

- (void)setUid:(NSString*)uid
{
    if (m_uid != nil) {
        [m_uid release];
    }
    m_uid = [[NSString alloc] initWithString:uid];
}

- (NSString*)getUid
{
    return m_uid;
}

@end

@implementation CFacebookLoadFriendsDelegate

- (id)init
{
    self = [super init];
    return self;
}

- (void)dealloc
{
    [super dealloc];
}

- (void)request:(FBRequest*)request didLoad:(id)result
{
    [m_FBManager didLoad:result requestType:FB_REQUEST_LOAD_FRIENDS uid:nil];
}

- (void)request:(FBRequest*)request didFailWithError:(NSError*)error
{
    [m_FBManager didFailWithError:error requestType:FB_REQUEST_LOAD_FRIENDS uid:nil];
}

@end

/** The application delegate forwards the Facebook login redirect URL. */
@interface AppDelegate (Extension)
- (BOOL)application:(UIApplication*)application handleOpenURL:(NSURL*)url;
@end

@implementation AppDelegate (Extension)

- (BOOL)application:(UIApplication*)application handleOpenURL:(NSURL*)url
{
    CFacebookInterface* facebookInterface = (CFacebookInterface*)CNGS_Platform::GetInstance()->getSocialInterface(SOCIAL_INTERFACE_FACEBOOK);
    return facebookInterface->handleOpenURL(url);
}

@end
