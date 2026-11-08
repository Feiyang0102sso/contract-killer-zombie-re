// CZombieMissionManager.cpp - construction of independently owned zombie missions.
#include "CZombieMissionManager.h"
#include "CZombieMission.h"

CZombieMissionManager::CZombieMissionManager()
{
}

CZombieMissionManager::~CZombieMissionManager()
{
    // Original releases only the vector storage; it does not delete mission pointers.
}

CZombieMission* CZombieMissionManager::Generate(CMission::EMissionType type,
                                               CMission::EMissionDifficulty difficulty,
                                               int waveCount, const XString& filter)
{
    return new CZombieMission(type, difficulty, waveCount, filter);
}
