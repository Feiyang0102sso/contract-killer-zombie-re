// GameCenter_Facade.h - Game Center wrapper used by the game: authentication, score / achievement
// posting (with a persistent retry queue), leaderboard loading and the leaderboard UI.
#ifndef GAMECENTER_FACADE_H
#define GAMECENTER_FACADE_H

#include "CSingleton.h"
#include "CStrWChar.h"
#include "CStrChar.h"

#ifdef __OBJC__
@class NSEnumerator;
@class NSArray;
@class NSString;
@class NSMutableDictionary;
#else
typedef struct objc_object NSEnumerator;
typedef struct objc_object NSArray;
typedef struct objc_object NSString;
typedef struct objc_object NSMutableDictionary;
#endif

/** One leaderboard to load: rows rangeStart..rangeEnd (1 based) of `category`. */
struct GameCenter_LeaderboardFilter {
    const char* identifier;   // 0x00 key used by GetLeaderboardSize / GetLeaderboardData
    const char* category;     // 0x04 Game Center leaderboard category
    int rangeStart;           // 0x08
    int rangeEnd;             // 0x0c
    int friendsOnly;          // 0x10 non-zero: GKLeaderboardPlayerScopeFriendsOnly
};

/** One row of a loaded leaderboard. */
struct GameCenter_LeaderboardData {
    com::glu::platform::components::CStrWChar name;   // 0x00 player alias
    com::glu::platform::components::CStrChar nameUtf8;  // 0x10 same alias as UTF-8
    int score;                                          // 0x20
    int rank;                                           // 0x24
};

class GameCenter_Facade : public com::glu::platform::components::CSingleton {
public:
    /** Singleton key (value taken from the original constructor). */
    static const unsigned int ClassId = 0x5847563b;

    /** Progress of an asynchronous request. */
    enum RequestState {
        RequestState_Running = 1,
        RequestState_Done = 2,
        RequestState_Failed = 3
    };

    /** Finds the registered instance or creates it. */
    static inline GameCenter_Facade* GetInstance()
    {
        void* found = NULL;
        CApplet::m_pApp->m_singletons->Find(ClassId, &found);
        GameCenter_Facade* instance = (GameCenter_Facade*)found;
        if (instance == NULL) {
            instance = new GameCenter_Facade();
        }
        return instance;
    }

    GameCenter_Facade();
    virtual ~GameCenter_Facade();

    bool isAuthenticated();
    // b081j: Player Update (original 14322a) reads the +8 authentication state directly.
    int GetAuthenticationState() const { return m_authenticationState; }
    void AuthenticatePlayer();
    /** GameKit present, iOS 4.1 or later and not an iPhone 3G. */
    bool isGameCenterSupported();
    void StartPostingScore(const char* category, int score);
    void StartLoadingLeaderboards(GameCenter_LeaderboardFilter* filters, int count);
    void StartPostingAchievement(const char* identifier, float percentComplete);
    /** Re-posts scores and achievements that failed to reach Game Center earlier. */
    void FetchPosting();
    int GetLeaderboardSize(const char* identifier);
    void GetLeaderboardData(const char* identifier, int index, GameCenter_LeaderboardData& data);
    void GetLocalPLayerName(com::glu::platform::components::CStrWChar& name);
    void GetLocalPlayerData(const char* identifier, GameCenter_LeaderboardData& data);
    void RunLeaderboardsView(const char* category);

private:
    void loadNextLeaderboard(NSEnumerator* enumerator);
    bool checkIfPlayerNamesNeedToBeLoaded();
    void initPlayersCache();
    void fillPlayersCache(NSArray* players);
    void tryRepostingAchievements();
    void tryRepostingScores();

    int m_authenticationState;                 // 0x08 RequestState
    int m_scorePostState;                      // 0x0c
    int m_leaderboardLoadState;                // 0x10
    int m_achievementPostState;                // 0x14
    int m_unused18;                            // 0x18 not used by this file
    NSMutableDictionary* m_leaderboards;       // 0x1c identifier -> GKLeaderboard
    NSMutableDictionary* m_playersCache;       // 0x20 playerID -> alias
    NSString* m_achievementsPath;              // 0x24 file of achievements not yet posted
    NSString* m_scoresPath;                    // 0x28 file of scores not yet posted
    NSMutableDictionary* m_postedAchievements; // 0x2c identifier -> NSNumber percent
    NSMutableDictionary* m_postedScores;       // 0x30 category -> NSNumber score
};

#endif
