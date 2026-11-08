// CGCLeaderboardsManager.h - Game Center leaderboards: posting the player's scores and loading the
// survival mode board (friends first, then everybody) for the leaderboard window.
#ifndef CGCLEADERBOARDSMANAGER_H
#define CGCLEADERBOARDSMANAGER_H

#include <stddef.h>
#include "xString.h"
#include "Vector_ss.h"

class CGCLeaderboardsManager {
public:
    /** One row of the loaded board. */
    struct SLeaderboardData {
        XString m_name;     // 0x00 player alias
        int m_score;        // 0x04
        int m_rank;         // 0x08
        bool m_isPlayer;    // 0x0c the local player
    };

    CGCLeaderboardsManager();

    /** Load the board step by step (m_state 0: request, 1: read, 2: done). */
    void Update();
    /** Rows of `board` (plus the local player when not among them). */
    void FillLeaderBoards(const char* board);
    void ReloadLeaderboards();
    /** Post all scores; false when Game Center is not available. */
    bool UpdateLeaderboards();

    static const char* BOARD_ZOMBIES_KILLED;
    static const char* BOARD_CIVILIANS_RESCUED;
    static const char* BOARD_XP;
    static const char* BOARD_HEADSHOTS;
    static const char* BOARD_MISSIONS_COMPLETED;
    static const char* BOARD_ACHIEVE;
    static const char* BOARD_BOARD_ACCURACY;
    static const char* BOARD_SURVIVAL_MODE_SCORE;

    int m_state;                                // 0x00
    Vector<SLeaderboardData*> m_leaderboards;   // 0x04
    bool m_isFriendsBoard;                      // 0x14
};

#endif
