// CGameCenterManager.h - GameKit side of the Game Center social interface: authentication, friends,
// scores, achievements and friend pictures. Results go back through CGameCenterInterface.
// Ivar layout and method types from the original Objective-C metadata.
#ifndef CGAMECENTERMANAGER_H
#define CGAMECENTERMANAGER_H

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <GameKit/GameKit.h>

@class CGameCenterManager;

/** Loads one friend picture and stores it in the manager's friend info. */
@interface CGameCenterUserPicDelegate : NSObject {
    NSString* m_uid;                    // +0x04 owned copy of the player id
    CGameCenterManager* m_gcMgr;        // +0x08
}

- (void)handleLoadUserPicResponse:(UIImage*)photo error:(NSError*)error;
- (void)setMgr:(CGameCenterManager*)manager;
- (void)setUid:(NSString*)uid;
- (NSString*)getUid;

@end

@interface CGameCenterManager : NSObject <GKLeaderboardViewControllerDelegate, GKAchievementViewControllerDelegate,
                                          GKFriendRequestComposeViewControllerDelegate> {
    int m_connectionState;                      // +0x04 0 idle, 1 busy, 2 done, 3 failed, 4 cancelled
    bool m_notificationRegistered;              // +0x08
    bool m_authenticated;                       // +0x09
    bool m_isLoggingIn;                         // +0x0a
    NSMutableArray* m_friendList;               // +0x0c player ids (the local player last)
    UIViewController* m_controller;             // +0x10 presenting the leaderboard / achievements
    UIViewController* m_requestController;      // +0x14 presenting the friend request screen
    BOOL m_loginAutomatically;                  // +0x18
    NSMutableDictionary* m_friendInfo;          // +0x1c player id -> {"player": GKPlayer, "picture": ...}
    NSMutableArray* m_gcUserPicDelegates;       // +0x20 picture loads in flight
}

- (int)getConnectionState;
- (void)reset;
- (void)clearData;
- (void)authenticate;
- (void)finishedAuthentication:(NSError*)error;
- (void)loadFriends;
- (void)finishedLoadingFriends:(NSError*)error;
- (void)finishedLoadingFriendsNames:(NSArray*)players error:(NSError*)error;
- (void)loadPlayersDetails:(NSArray*)identifiers;
- (void)finishedLoadingPlayers:(NSError*)error withDetails:(NSArray*)players;
- (void)userAuthenticationChange:(NSNotification*)notification;
- (void)playerDataChange:(NSNotification*)notification;
- (void)handleNotification;
- (bool)isUserAuthenticated;
- (NSString*)getLocalUserID;
- (NSString*)getLocalUserAlias;
- (int)getFriendsCount;
- (NSMutableArray*)getFriends;
- (void)loadCategoryTitles;
- (void)reportScore:(int64_t)score forCategory:(NSString*)category;
- (void)showLeaderBoard:(UIViewController*)viewController;
- (void)showAchievement:(UIViewController*)viewController;
- (void)loadAchievements;
- (void)loadAchievementDescriptions;
- (void)reportAchievementIdentifier:(NSString*)identifier percentComplete:(float)percent;
- (void)showFriendRequestScreen:(UIViewController*)viewController;
- (NSMutableDictionary*)getFriendsInfo;
/** Start loading the picture of `uid` ("normal" or small `type`); NO when it cannot be loaded. */
- (BOOL)loadUserPic:(NSString*)uid type:(NSString*)type;
- (void)removeUserPicDelegate:(NSString*)uid;

@end

#endif
