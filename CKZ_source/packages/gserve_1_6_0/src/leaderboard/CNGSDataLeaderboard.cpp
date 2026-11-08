// CNGSDataLeaderboard.cpp - leaderboard data of gServe.
// The functions of this object were dead-stripped by the linker; only the frequency name table
// (and its static constructor / destructor) survived.
#include "CNGSDataLeaderboard.h"

using com::glu::platform::components::CStrWChar;

CStrWChar LeaderboardFrequencies[LEADERBOARD_FREQUENCY_COUNT] = {
    L"CUMULATIVE",
    L"ONE_DAY",
    L"SEVEN_DAY",
    L"THIRTY_DAY"
};
