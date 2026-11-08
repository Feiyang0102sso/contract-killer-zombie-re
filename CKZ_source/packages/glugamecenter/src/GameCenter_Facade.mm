// GameCenter_Facade.mm - Game Center (GameKit, iOS 4.1+) wrapper: authentication, score and
// achievement posting with a persistent retry queue, leaderboard loading and the leaderboard view.
#include "GameCenter_Facade.h"

#import <UIKit/UIKit.h>
#import <GameKit/GameKit.h>

#include <sys/types.h>
#include <sys/sysctl.h>

// the hw.machine buffer below comes straight from the C library
#undef malloc
#undef free

/** Dismisses the achievement view and releases itself. */
@interface MyAchievementViewControllerDelegate : NSObject <GKAchievementViewControllerDelegate>
@end

/** Dismisses the leaderboard view and releases itself. */
@interface MyLeaderboardViewControllerDelegate : NSObject <GKLeaderboardViewControllerDelegate>
@end

/** Delegate of the leaderboard view currently on screen (created by RunLeaderboardsView). */
MyLeaderboardViewControllerDelegate* m_leaderboardViewControllerDelegate;

@implementation MyAchievementViewControllerDelegate

- (void) achievementViewControllerDidFinish: (GKAchievementViewController*) viewController
{
    UIWindow* window = [[UIApplication sharedApplication] keyWindow];
    UIViewController* rootViewController = [window rootViewController];
    [viewController dismissModalViewControllerAnimated:NO];
    [window setRootViewController:nil];
    [rootViewController release];
    [self release];
}

@end

@implementation MyLeaderboardViewControllerDelegate

- (void) leaderboardViewControllerDidFinish: (GKLeaderboardViewController*) viewController
{
    UIWindow* window = [[UIApplication sharedApplication] keyWindow];
    UIViewController* rootViewController = [window rootViewController];
    [viewController dismissModalViewControllerAnimated:NO];
    [window setRootViewController:nil];
    [rootViewController release];
    [self release];
}

@end

GameCenter_Facade::GameCenter_Facade()
    : CSingleton(ClassId)
{
    m_leaderboards = nil;
    m_playersCache = nil;

    // Scores / achievements not yet accepted by Game Center survive restarts in the documents folder
    NSString* documentsPath = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0];
    m_achievementsPath = [[documentsPath stringByAppendingPathComponent:@"posted_achievements"] retain];
    m_scoresPath = [[documentsPath stringByAppendingPathComponent:@"posted_scores"] retain];

    m_postedAchievements = [NSMutableDictionary dictionaryWithContentsOfFile:m_achievementsPath];
    if (m_postedAchievements == nil) {
        m_postedAchievements = [NSMutableDictionary dictionaryWithCapacity:10];
    }
    [m_postedAchievements retain];

    m_postedScores = [NSMutableDictionary dictionaryWithContentsOfFile:m_scoresPath];
    if (m_postedScores == nil) {
        m_postedScores = [NSMutableDictionary dictionaryWithCapacity:5];
    }
    [m_postedScores retain];
}

GameCenter_Facade::~GameCenter_Facade()
{
    if (m_playersCache != nil) {
        [m_playersCache removeAllObjects];
        [m_playersCache release];
    }
    if (m_leaderboards != nil) {
        [m_leaderboards removeAllObjects];
        [m_leaderboards release];
    }
    [m_achievementsPath release];
    [m_postedAchievements release];
    [m_scoresPath release];
    [m_postedScores release];
}

bool GameCenter_Facade::isAuthenticated()
{
    return [[GKLocalPlayer localPlayer] isAuthenticated];
}

void GameCenter_Facade::AuthenticatePlayer()
{
    if (isGameCenterSupported()) {
        m_authenticationState = RequestState_Running;
        [[GKLocalPlayer localPlayer] authenticateWithCompletionHandler:^(NSError* error) {
            if (error == nil) {
                m_authenticationState = RequestState_Done;
            } else {
                m_authenticationState = RequestState_Failed;
            }
        }];
    }
}

void GameCenter_Facade::StartPostingScore(const char* category, int score)
{
    if (isGameCenterSupported()) {
        m_scorePostState = RequestState_Running;
        NSString* categoryString = [NSString stringWithCString:category encoding:NSUTF8StringEncoding];
        GKScore* scoreReporter = [[GKScore alloc] initWithCategory:categoryString];
        [scoreReporter setValue:score];

        // Remember the score until Game Center confirms it
        NSNumber* scoreNumber = [NSNumber numberWithInt:score];
        [m_postedScores setObject:scoreNumber forKey:categoryString];
        [m_postedScores writeToFile:m_scoresPath atomically:YES];

        [scoreReporter reportScoreWithCompletionHandler:^(NSError* error) {
            if (error == nil) {
                m_scorePostState = RequestState_Done;
                [m_postedScores removeObjectForKey:categoryString];
                [m_postedScores writeToFile:m_scoresPath atomically:YES];
            } else {
                m_scorePostState = RequestState_Failed;
            }
        }];
        [scoreReporter release];
    }
}

/** Loads the scores of the leaderboards one after another, then the aliases of their players. */
void GameCenter_Facade::loadNextLeaderboard(NSEnumerator* enumerator)
{
    GKLeaderboard* leaderboard = [enumerator nextObject];
    if (leaderboard != nil) {
        [leaderboard loadScoresWithCompletionHandler:^(NSArray* scores, NSError* error) {
            if (error == nil) {
                loadNextLeaderboard(enumerator);
            } else {
                m_leaderboardLoadState = RequestState_Failed;
                [m_leaderboards removeAllObjects];
                [enumerator release];
            }
        }];
    } else {
        if (checkIfPlayerNamesNeedToBeLoaded()) {
            initPlayersCache();
            [GKPlayer loadPlayersForIdentifiers:[m_playersCache allKeys] withCompletionHandler:^(NSArray* players, NSError* error) {
                if (error == nil) {
                    fillPlayersCache(players);
                    m_leaderboardLoadState = RequestState_Done;
                } else {
                    m_leaderboardLoadState = RequestState_Failed;
                }
            }];
        } else {
            m_leaderboardLoadState = RequestState_Done;
        }
        [enumerator release];
    }
}

void GameCenter_Facade::StartLoadingLeaderboards(GameCenter_LeaderboardFilter* filters, int count)
{
    if (isGameCenterSupported() && m_leaderboardLoadState != RequestState_Running) {
        m_leaderboardLoadState = RequestState_Running;
        if (m_leaderboards == nil) {
            m_leaderboards = [[NSMutableDictionary alloc] initWithCapacity:5];
        }
        [m_leaderboards removeAllObjects];

        // Size the alias cache for the longest requested range
        int maxRows = 0;
        for (int index = 0; index < count; index++) {
            int rows = filters[index].rangeEnd - filters[index].rangeStart + 1;
            if (rows > maxRows) {
                maxRows = rows;
            }
        }
        if (m_playersCache == nil) {
            m_playersCache = [[NSMutableDictionary alloc] initWithCapacity:maxRows * count];
        }

        for (int index = 0; index < count; index++) {
            NSString* identifier = [NSString stringWithCString:filters[index].identifier encoding:NSUTF8StringEncoding];
            NSString* category = [NSString stringWithCString:filters[index].category encoding:NSUTF8StringEncoding];
            GKLeaderboard* leaderboard = [[GKLeaderboard alloc] init];
            [leaderboard setCategory:category];
            [leaderboard setRange:NSMakeRange(filters[index].rangeStart, filters[index].rangeEnd)];
            // ?: kept: the original converts the flag with a conditional move
            [leaderboard setPlayerScope:(filters[index].friendsOnly != 0) ? GKLeaderboardPlayerScopeFriendsOnly : GKLeaderboardPlayerScopeGlobal];
            [m_leaderboards setObject:leaderboard forKey:identifier];
        }

        loadNextLeaderboard([[m_leaderboards objectEnumerator] retain]);
    }
}

void GameCenter_Facade::StartPostingAchievement(const char* identifier, float percentComplete)
{
    if (isGameCenterSupported()) {
        m_achievementPostState = RequestState_Running;
        NSString* identifierString = [NSString stringWithCString:identifier encoding:NSUTF8StringEncoding];

        // Remember the achievement until Game Center confirms it
        NSNumber* percentNumber = [NSNumber numberWithFloat:percentComplete];
        [m_postedAchievements setObject:percentNumber forKey:identifierString];
        [m_postedAchievements writeToFile:m_achievementsPath atomically:YES];

        GKAchievement* achievement = [[[GKAchievement alloc] initWithIdentifier:identifierString] autorelease];
        if (achievement != nil) {
            [achievement setPercentComplete:percentComplete];
            [achievement reportAchievementWithCompletionHandler:^(NSError* error) {
                if (error == nil) {
                    m_achievementPostState = RequestState_Done;
                    [m_postedAchievements removeObjectForKey:identifierString];
                    [m_postedAchievements writeToFile:m_achievementsPath atomically:YES];
                } else {
                    m_achievementPostState = RequestState_Failed;
                }
            }];
        }
    }
}

void GameCenter_Facade::FetchPosting()
{
    if (isGameCenterSupported()) {
        tryRepostingAchievements();
        tryRepostingScores();
    }
}

void GameCenter_Facade::tryRepostingAchievements()
{
    NSEnumerator* enumerator = [m_postedAchievements keyEnumerator];
    NSString* identifier;
    while ((identifier = [enumerator nextObject]) != nil) {
        NSNumber* percentNumber = [m_postedAchievements objectForKey:identifier];
        GKAchievement* achievement = [[[GKAchievement alloc] initWithIdentifier:identifier] autorelease];
        if (achievement != nil) {
            [achievement setPercentComplete:[percentNumber floatValue]];
            [achievement reportAchievementWithCompletionHandler:^(NSError* error) {
                if (error == nil) {
                    [m_postedAchievements removeObjectForKey:identifier];
                    [m_postedAchievements writeToFile:m_achievementsPath atomically:YES];
                }
            }];
        }
    }
}

void GameCenter_Facade::tryRepostingScores()
{
    NSEnumerator* enumerator = [m_postedScores keyEnumerator];
    NSString* category;
    while ((category = [enumerator nextObject]) != nil) {
        NSNumber* scoreNumber = [m_postedScores objectForKey:category];
        GKScore* scoreReporter = [[[GKScore alloc] initWithCategory:category] autorelease];
        if (scoreReporter != nil) {
            [scoreReporter setValue:[scoreNumber intValue]];
            [scoreReporter reportScoreWithCompletionHandler:^(NSError* error) {
                if (error == nil) {
                    [m_postedScores removeObjectForKey:category];
                    [m_postedScores writeToFile:m_scoresPath atomically:YES];
                }
            }];
        }
    }
}

int GameCenter_Facade::GetLeaderboardSize(const char* identifier)
{
    if (m_leaderboardLoadState == RequestState_Done) {
        NSString* identifierString = [NSString stringWithCString:identifier encoding:NSUTF8StringEncoding];
        return [[[m_leaderboards objectForKey:identifierString] scores] count];
    }
    return 0;
}

void GameCenter_Facade::GetLeaderboardData(const char* identifier, int index, GameCenter_LeaderboardData& data)
{
    if (m_leaderboardLoadState != RequestState_Done) {
        return;
    }

    NSString* identifierString = [NSString stringWithCString:identifier encoding:NSUTF8StringEncoding];
    GKScore* score = [[[m_leaderboards objectForKey:identifierString] scores] objectAtIndex:index];
    NSString* alias = [m_playersCache objectForKey:[score playerID]];
    if (alias == nil) {
        alias = [NSString stringWithCString:"?????" encoding:NSUTF8StringEncoding];
    }

    if ([alias canBeConvertedToEncoding:NSASCIIStringEncoding]) {
        char buffer[100];
        buffer[0] = '\0';
        [alias getCString:buffer maxLength:100 encoding:NSUTF8StringEncoding];
        data.name = buffer;
        data.nameUtf8 = buffer;
    } else if ([[[GKLocalPlayer localPlayer] playerID] isEqual:[score playerID]]) {
        // Non-ASCII alias: only the local player gets a readable name
        data.name = "YOU";
        data.nameUtf8 = "YOU";
    } else {
        data.name = "?????";
        data.nameUtf8 = "?????";
    }
    data.score = [score value];
    data.rank = [score rank];
}

void GameCenter_Facade::GetLocalPLayerName(com::glu::platform::components::CStrWChar& name)
{
    if ([[GKLocalPlayer localPlayer] isAuthenticated]) {
        GKLocalPlayer* localPlayer = [GKLocalPlayer localPlayer];
        if ([[localPlayer alias] canBeConvertedToEncoding:NSASCIIStringEncoding]) {
            char buffer[100];
            [[localPlayer alias] getCString:buffer maxLength:100 encoding:NSUTF8StringEncoding];
            name = buffer;
        }
    }
}

void GameCenter_Facade::GetLocalPlayerData(const char* identifier, GameCenter_LeaderboardData& data)
{
    if (m_leaderboardLoadState != RequestState_Done) {
        return;
    }

    NSString* identifierString = [NSString stringWithCString:identifier encoding:NSUTF8StringEncoding];
    GKScore* score = [[m_leaderboards objectForKey:identifierString] localPlayerScore];
    GKLocalPlayer* localPlayer = [GKLocalPlayer localPlayer];
    if ([[localPlayer alias] canBeConvertedToEncoding:NSASCIIStringEncoding]) {
        char buffer[100];
        [[localPlayer alias] getCString:buffer maxLength:100 encoding:NSUTF8StringEncoding];
        data.name = buffer;
        data.nameUtf8 = buffer;
    } else {
        data.name = "YOU";
        data.nameUtf8 = "YOU";
    }
    data.score = [score value];
    data.rank = [score rank];
}

void GameCenter_Facade::RunLeaderboardsView(const char* category)
{
    if (isGameCenterSupported()) {
        m_leaderboardViewControllerDelegate = [[MyLeaderboardViewControllerDelegate alloc] init];
        GKLeaderboardViewController* leaderboardController = [[GKLeaderboardViewController alloc] init];
        if (leaderboardController != nil) {
            if (category != NULL) {
                [leaderboardController setCategory:[NSString stringWithCString:category encoding:NSUTF8StringEncoding]];
            }
            [leaderboardController setLeaderboardDelegate:m_leaderboardViewControllerDelegate];
            id applicationDelegate = [[UIApplication sharedApplication] delegate];
            if ([applicationDelegate respondsToSelector:@selector(viewController)]) {
                [[applicationDelegate viewController] presentModalViewController:leaderboardController animated:YES];
            }
        }
        [leaderboardController release];
    }
}

// ---- helpers ----
// Defined after the public API as in the original file: the definition order decides the uniquing
// numbers of the block helper symbols (___block_descriptor_tmpNN, ___copy_helper_block_NN).

bool GameCenter_Facade::isGameCenterSupported()
{
    // GameKit class present
    Class localPlayerClass = NSClassFromString(@"GKLocalPlayer");

    // iOS 4.1 or later
    NSString* requiredVersion = @"4.1";
    NSString* systemVersion = [[UIDevice currentDevice] systemVersion];
    bool versionSupported = [systemVersion compare:requiredVersion options:NSNumericSearch] != NSOrderedAscending;

    // Game Center does not run on the iPhone 3G
    size_t size;
    sysctlbyname("hw.machine", NULL, &size, NULL, 0);
    char* machine = (char*)malloc(size);
    sysctlbyname("hw.machine", machine, &size, NULL, 0);
    NSString* platform = [NSString stringWithCString:machine encoding:NSASCIIStringEncoding];
    free(machine);
    bool isIPhone3G = [platform isEqualToString:@"iPhone1,2"];

    return localPlayerClass != nil && versionSupported && !isIPhone3G;
}

/** True when a loaded score belongs to a player whose alias is not cached yet. */
bool GameCenter_Facade::checkIfPlayerNamesNeedToBeLoaded()
{
    NSEnumerator* enumerator = [m_leaderboards objectEnumerator];
    GKLeaderboard* leaderboard;
    while ((leaderboard = [enumerator nextObject]) != nil) {
        for (NSUInteger index = 0; index < [[leaderboard scores] count]; index++) {
            GKScore* score = [[leaderboard scores] objectAtIndex:index];
            if ([m_playersCache objectForKey:[score playerID]] == nil) {
                return true;
            }
        }
    }
    return false;
}

/** Adds a "???" placeholder for every player without a cached alias. */
void GameCenter_Facade::initPlayersCache()
{
    NSEnumerator* enumerator = [m_leaderboards objectEnumerator];
    GKLeaderboard* leaderboard;
    while ((leaderboard = [enumerator nextObject]) != nil) {
        for (NSUInteger index = 0; index < [[leaderboard scores] count]; index++) {
            GKScore* score = [[leaderboard scores] objectAtIndex:index];
            if ([m_playersCache objectForKey:[score playerID]] == nil) {
                [m_playersCache setObject:@"???" forKey:[score playerID]];
            }
        }
    }
}

void GameCenter_Facade::fillPlayersCache(NSArray* players)
{
    for (NSUInteger index = 0; index < [players count]; index++) {
        GKPlayer* player = [players objectAtIndex:index];
        [m_playersCache setObject:[player alias] forKey:[player playerID]];
    }
}
