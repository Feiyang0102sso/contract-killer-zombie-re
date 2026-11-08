// CGameCenterManager.mm - GameKit side of the Game Center social interface.
#import "CGameCenterManager.h"
#import "CUtil.h"
#include "CNGS_Platform.h"
#include "CGameCenterInterface.h"

using com::glu::platform::core::CVector;

/** The Game Center social interface (all results are reported to it). */
static inline CNGSSocialInterface* GameCenterInterface()
{
    return CNGS_Platform::GetInstance()->getSocialInterface(SOCIAL_INTERFACE_GAME_CENTER);
}

@implementation CGameCenterManager

- (id)init
{
    self = [super init];
    if (self != nil) {
        [self reset];
        m_notificationRegistered = false;
        m_authenticated = false;
        m_isLoggingIn = false;
        m_loginAutomatically = YES;
        m_friendInfo = [[NSMutableDictionary alloc] init];
        m_friendList = [[NSMutableArray alloc] init];
        m_gcUserPicDelegates = [[NSMutableArray alloc] init];
    }
    return self;
}

- (void)dealloc
{
    if (m_notificationRegistered) {
        [[NSNotificationCenter defaultCenter] removeObserver:self name:GKPlayerAuthenticationDidChangeNotificationName object:nil];
        m_notificationRegistered = false;
        m_authenticated = false;
    }
    [m_gcUserPicDelegates release];
    [m_friendList release];
    [m_friendInfo release];
    [super dealloc];
}

- (int)getConnectionState
{
    return m_connectionState;
}

- (void)reset
{
    m_connectionState = 0;
}

- (void)clearData
{
    [m_friendList removeAllObjects];
    [m_friendInfo removeAllObjects];
}

- (void)authenticate
{
    m_connectionState = 1;
    m_isLoggingIn = true;
    if (!m_notificationRegistered) {
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(userAuthenticationChange:)
                                                     name:GKPlayerAuthenticationDidChangeNotificationName object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(playerDataChange:)
                                                     name:GKPlayerDidChangeNotificationName object:nil];
        m_notificationRegistered = true;
    }
    [[GKLocalPlayer localPlayer] authenticateWithCompletionHandler:^(NSError* error) {
        [self finishedAuthentication:error];
    }];
}

- (void)finishedAuthentication:(NSError*)error
{
    if (!m_isLoggingIn) {
        return;
    }
    m_isLoggingIn = false;
    if (error == nil || [error code] == GKErrorAuthenticationInProgress) {
        m_connectionState = 2;
        m_loginAutomatically = YES;
    } else {
        NSLog(@"error: %@", error);
        if ([error code] == GKErrorCancelled) {
            m_connectionState = 4;
        } else {
            m_connectionState = 3;
        }
    }
    m_authenticated = [self isUserAuthenticated];
    GameCenterInterface()->finishLogin();
}

- (void)loadFriends
{
    m_connectionState = 1;
    [m_friendInfo removeAllObjects];
    [[GKLocalPlayer localPlayer] loadFriendsWithCompletionHandler:^(NSArray* friends, NSError* error) {
        [self finishedLoadingFriends:error];
    }];
}

- (void)finishedLoadingFriends:(NSError*)error
{
    [m_friendInfo removeAllObjects];
    if (error == nil) {
        m_connectionState = 2;
        [m_friendList removeAllObjects];
        [m_friendList addObjectsFromArray:[CUtil sortByName:[[GKLocalPlayer localPlayer] friends]]];
        [m_friendList addObject:[self getLocalUserID]];
        [GKPlayer loadPlayersForIdentifiers:m_friendList withCompletionHandler:^(NSArray* players, NSError* loadError) {
            [self finishedLoadingFriendsNames:players error:loadError];
        }];
    } else {
        m_connectionState = 3;
        GameCenterInterface()->finishLoadFriends();
    }
}

- (void)finishedLoadingFriendsNames:(NSArray*)players error:(NSError*)error
{
    if (error == nil) {
        CVector<FriendInfo>* friendsInfos = GameCenterInterface()->getSocialNetworkFriendsInfos();
        friendsInfos->Clear();
        for (unsigned int index = 0; index < [players count]; index++) {
            GKPlayer* player = [players objectAtIndex:index];
            NSString* alias = [player alias];
            NSString* playerID = [player playerID];
            if ([playerID compare:[self getLocalUserID]] != NSOrderedSame) {
                // Keep the list sorted by name: insert before the first name sorting after this one.
                int position;
                for (position = 0; position < friendsInfos->Size(); position++) {
                    NSString* name = [CUtil WcharToNSString:(*friendsInfos)[position].m_name.m_string];
                    if ([alias caseInsensitiveCompare:name] == NSOrderedAscending) {
                        break;
                    }
                }
                FriendInfo info;
                info.m_name = [CUtil NSStringToWchar:alias];
                info.m_id = [CUtil NSStringToWchar:playerID];
                friendsInfos->Insert(position, info);
            }
            NSMutableDictionary* entry = [[[NSMutableDictionary alloc] init] autorelease];
            [entry setObject:player forKey:@"player"];
            [m_friendInfo setObject:entry forKey:playerID];
        }
        // The local player was only in the list to get its name loaded.
        for (unsigned int index = 0; index < [m_friendList count]; index++) {
            if ([(NSString*)[m_friendList objectAtIndex:index] compare:[self getLocalUserID]] == NSOrderedSame) {
                [m_friendList removeObjectAtIndex:index];
            }
        }
    }
    GameCenterInterface()->finishLoadFriends();
}

- (void)loadPlayersDetails:(NSArray*)identifiers
{
    m_connectionState = 1;
    GKLocalPlayer* localPlayer = [GKLocalPlayer localPlayer];
    [GKLocalPlayer loadPlayersForIdentifiers:[localPlayer friends] withCompletionHandler:^(NSArray* players, NSError* error) {
        [self finishedLoadingPlayers:error withDetails:players];
    }];
}

- (void)finishedLoadingPlayers:(NSError*)error withDetails:(NSArray*)players
{
    if (error == nil) {
        m_connectionState = 2;
    } else {
        m_connectionState = 3;
    }
}

- (void)userAuthenticationChange:(NSNotification*)notification
{
    [self handleNotification];
}

- (void)playerDataChange:(NSNotification*)notification
{
    ((CGameCenterInterface*)GameCenterInterface())->handleFriendsNotification();
}

- (void)handleNotification
{
    if (m_isLoggingIn) {
        return;
    }
    if (m_authenticated && ![self isUserAuthenticated]) {
        // Signed out of Game Center.
        GameCenterInterface()->finishLogout();
    } else {
        if ([self isUserAuthenticated] == true) {
            m_loginAutomatically = NO;
            ((CGameCenterInterface*)GameCenterInterface())->handleNotification();
        } else {
            ((CGameCenterInterface*)GameCenterInterface())->handleNotification();
        }
    }
    m_authenticated = [self isUserAuthenticated];
}

- (bool)isUserAuthenticated
{
    return [[GKLocalPlayer localPlayer] isAuthenticated];
}

- (NSString*)getLocalUserID
{
    return [[GKLocalPlayer localPlayer] playerID];
}

- (NSString*)getLocalUserAlias
{
    return [[GKLocalPlayer localPlayer] alias];
}

- (int)getFriendsCount
{
    return [m_friendList count];
}

- (NSMutableArray*)getFriends
{
    return m_friendList;
}

- (void)loadCategoryTitles
{
    [GKLeaderboard loadCategoriesWithCompletionHandler:^(NSArray* categories, NSArray* titles, NSError* error) {
    }];
}

- (void)reportScore:(int64_t)score forCategory:(NSString*)category
{
    GKScore* scoreReporter = [[[GKScore alloc] initWithCategory:category] autorelease];
    scoreReporter.value = score;
    [scoreReporter reportScoreWithCompletionHandler:^(NSError* error) {
        if (error == nil) {
            m_connectionState = 2;
        } else {
            m_connectionState = 3;
        }
        GameCenterInterface()->finishReportScore();
    }];
}

- (void)showLeaderBoard:(UIViewController*)viewController
{
    GKLeaderboardViewController* leaderboardController = [[GKLeaderboardViewController alloc] init];
    if (leaderboardController != nil) {
        leaderboardController.leaderboardDelegate = self;
        m_controller = viewController;
        [m_controller presentModalViewController:leaderboardController animated:YES];
    }
    [leaderboardController release];
}

- (void)leaderboardViewControllerDidFinish:(GKLeaderboardViewController*)viewController
{
    [m_controller dismissModalViewControllerAnimated:YES];
    m_controller = nil;
}

- (void)showAchievement:(UIViewController*)viewController
{
    GKAchievementViewController* achievementController = [[GKAchievementViewController alloc] init];
    if (achievementController != nil) {
        achievementController.achievementDelegate = self;
        m_controller = viewController;
        [m_controller presentModalViewController:achievementController animated:YES];
    }
    [achievementController release];
}

- (void)achievementViewControllerDidFinish:(GKAchievementViewController*)viewController
{
    [m_controller dismissModalViewControllerAnimated:YES];
    m_controller = nil;
}

- (void)loadAchievements
{
    [GKAchievement loadAchievementsWithCompletionHandler:^(NSArray* achievements, NSError* error) {
        if (error == nil) {
            m_connectionState = 2;
        } else {
            m_connectionState = 3;
        }
    }];
}

- (void)loadAchievementDescriptions
{
    [GKAchievementDescription loadAchievementDescriptionsWithCompletionHandler:^(NSArray* descriptions, NSError* error) {
    }];
}

- (void)reportAchievementIdentifier:(NSString*)identifier percentComplete:(float)percent
{
    GKAchievement* achievement = [[[GKAchievement alloc] initWithIdentifier:identifier] autorelease];
    if (achievement != nil) {
        achievement.percentComplete = percent;
        [achievement reportAchievementWithCompletionHandler:^(NSError* error) {
            if (error == nil) {
                m_connectionState = 2;
            } else {
                m_connectionState = 3;
            }
            GameCenterInterface()->finishReportAchievement();
        }];
    }
}

- (void)showFriendRequestScreen:(UIViewController*)viewController
{
    GKFriendRequestComposeViewController* requestController = [[GKFriendRequestComposeViewController alloc] init];
    if (requestController != nil) {
        requestController.composeViewDelegate = self;
        m_requestController = viewController;
        [m_requestController presentModalViewController:requestController animated:YES];
    }
    [requestController release];
}

- (void)friendRequestComposeViewControllerDidFinish:(GKFriendRequestComposeViewController*)viewController
{
    [m_requestController dismissModalViewControllerAnimated:YES];
    m_requestController = nil;
}

- (NSMutableDictionary*)getFriendsInfo
{
    return m_friendInfo;
}

- (BOOL)loadUserPic:(NSString*)uid type:(NSString*)type
{
    NSLog(@"%@, %@", uid, type);
    BOOL started = NO;
    if ([m_gcUserPicDelegates count] < 10) {
        // Already loading this one.
        for (unsigned int index = 0; index < [m_gcUserPicDelegates count]; index++) {
            if ([[[m_gcUserPicDelegates objectAtIndex:index] getUid] compare:uid] == NSOrderedSame) {
                return YES;
            }
        }
        NSMutableDictionary* info = [m_friendInfo objectForKey:uid];
        if (info != nil) {
            GKPlayer* player = [info objectForKey:@"player"];
            if (player != nil) {
                if ([player respondsToSelector:@selector(loadPhotoForSize:withCompletionHandler:)]) {
                    GKPhotoSize photoSize = ([type compare:@"normal"] == NSOrderedSame);
                    CGameCenterUserPicDelegate* picDelegate = [[CGameCenterUserPicDelegate alloc] init];
                    [picDelegate setMgr:self];
                    [picDelegate setUid:uid];
                    [player loadPhotoForSize:photoSize withCompletionHandler:^(UIImage* photo, NSError* error) {
                        [picDelegate handleLoadUserPicResponse:photo error:error];
                    }];
                    started = YES;
                    [m_gcUserPicDelegates addObject:picDelegate];
                } else {
                    // Before iOS 5 GameKit has no player photos.
                    [info setObject:@"error" forKey:@"picture"];
                    GameCenterInterface()->finishLoadFriendPic(0);
                }
            }
        }
    }
    return started;
}

- (void)removeUserPicDelegate:(NSString*)uid
{
    for (unsigned int index = 0; index < [m_gcUserPicDelegates count]; index++) {
        CGameCenterUserPicDelegate* picDelegate = [m_gcUserPicDelegates objectAtIndex:index];
        if ([[picDelegate getUid] compare:uid] == NSOrderedSame) {
            [m_gcUserPicDelegates removeObject:picDelegate];
            [picDelegate release];
            return;
        }
    }
}

@end

@implementation CGameCenterUserPicDelegate

- (void)handleLoadUserPicResponse:(UIImage*)photo error:(NSError*)error
{
    NSMutableDictionary* info = [[m_gcMgr getFriendsInfo] objectForKey:m_uid];
    unsigned char success = 0;
    if (info != nil) {
        [info objectForKey:@"picture"];
        if (error == nil && photo != nil) {
            [info setObject:UIImagePNGRepresentation(photo) forKey:@"picture"];
            success = 1;
        } else {
            success = 0;
            [info setObject:@"error" forKey:@"picture"];
        }
    }
    [m_gcMgr removeUserPicDelegate:m_uid];
    GameCenterInterface()->finishLoadFriendPic(success);
}

- (void)setMgr:(CGameCenterManager*)manager
{
    m_gcMgr = manager;
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

- (id)init
{
    self = [super init];
    m_uid = nil;
    m_gcMgr = nil;
    return self;
}

- (void)dealloc
{
    [m_uid release];
    [super dealloc];
}

@end
