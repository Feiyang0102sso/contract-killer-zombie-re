// CNGSDataLeaderboard.h - leaderboard data of gServe (b023x).
// Only the frequency names survived the linker's dead stripping.
#ifndef CNGSDATALEADERBOARD_H
#define CNGSDATALEADERBOARD_H

#include "CStrWChar.h"

/** Leaderboard periods (index into LeaderboardFrequencies). */
enum eLeaderboardFrequency {
    LEADERBOARD_FREQUENCY_CUMULATIVE = 0,
    LEADERBOARD_FREQUENCY_ONE_DAY = 1,
    LEADERBOARD_FREQUENCY_SEVEN_DAY = 2,
    LEADERBOARD_FREQUENCY_THIRTY_DAY = 3,
    LEADERBOARD_FREQUENCY_COUNT = 4
};

/** Server names of the periods. */
extern com::glu::platform::components::CStrWChar LeaderboardFrequencies[LEADERBOARD_FREQUENCY_COUNT];

#endif
