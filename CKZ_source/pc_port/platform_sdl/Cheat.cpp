// Recognise the cheat codes of Cheat.h in the typed letters and apply them to the player.
#include "Cheat.h"
#include <stdio.h>
#include <string.h>
#include <string>
#include "App.h"
#include "CBH_Player.h"
#include "CGPSMapGame.h"
#include "CGPSMapScreen.h"
#include "CHuntingInfo.h"
#include "CProgressSystem.h"
#include "CTutorialManager.h"
#include "DGHelper.h"

namespace ckz {
namespace {

// Index of the experience value in the player stats (CBH_Player::AddXp uses the same one).
constexpr PLAYER_STATS_TYPE PlayerStatsXp = static_cast<PLAYER_STATS_TYPE>(3);

// The last typed letters, oldest first.
std::string typedLetters;

/**
 * Show the level-up animation. The game itself only plays it when the map opens after a mission
 * that raised the level (CGPSMapGame::Init), so do the same here.
 */
void ShowLevelUp(App& app)
{
    CGPSMapScreen* mapScreen = app.m_helper->m_gpsScreen;
    const bool inMission = app.m_helper->m_swerveGame != nullptr;
    const bool mapOpen = !inMission && mapScreen != nullptr && mapScreen->m_mapGame != nullptr &&
        mapScreen->m_mapGame->m_parent == mapScreen;
    if (!mapOpen) {
        // Not on the map: leave the game's own flag set, the map plays the animation when it opens.
        app.m_huntingInfo->m_levelIncreased = true;
        fprintf(stderr, "Cheat: level-up animation deferred until the map opens\n");
        return;
    }
    CGPSMapGame* map = mapScreen->m_mapGame;
    map->AddLevelUpDialogInQueue(0);
    if (app.m_tutorialManager->IsPlayerInTutorial() == 0) {
        map->AddRecommendedWeaponDialogInQueue();
    }
    fprintf(stderr, "Cheat: level-up animation queued on the map\n");
}

/** Raise the player level through the game's own experience path, so its level-up handling runs. */
void AddLevels(int levels)
{
    auto* app = static_cast<App*>(WindowApp::m_instance);
    CBH_Player* player = CBH_Player::GetInstance();
    const int oldLevel = player->GetLevel();
    int targetLevel = oldLevel + levels;
    const int maxLevel = app->m_progressSystem->GetMaxLevel();
    if (targetLevel > maxLevel) {
        targetLevel = maxLevel;
    }
    // A level is reached at its experience threshold: give exactly what is missing.
    const int missingXp = app->m_progressSystem->GetMaxXPForLevel(targetLevel) - player->GetStats(PlayerStatsXp);
    if (missingXp > 0) {
        player->AddXp(missingXp);
    }
    fprintf(stderr, "Cheat: level %d -> %d (max %d)\n", oldLevel, player->GetLevel(), maxLevel);
    if (player->GetLevel() > oldLevel) {
        ShowLevelUp(*app);
    }
}

void AddCurrencies(int amount)
{
    CBH_Player* player = CBH_Player::GetInstance();
    player->AddMoney(amount);
    player->AddCredits(amount, L"cheat");
    fprintf(stderr, "Cheat: cash %d, credits %d\n", player->GetStats(PLAYER_STATS_SOFT_CURRENCY),
            player->GetStats(PLAYER_STATS_HARD_CURRENCY));
}

void ApplyCheat(const CheatCode& code)
{
    // The player and the level table only exist once the game has loaded.
    if (!App::IsInitialized()) {
        fprintf(stderr, "Cheat %s ignored: game not loaded yet\n", code.text);
        return;
    }
    switch (code.effect) {
    case CheatEffect::AddLevels:
        AddLevels(code.amount);
        break;
    case CheatEffect::AddCurrencies:
        AddCurrencies(code.amount);
        break;
    }
}

/** True when the typed letters end with the code text. */
bool TypedLettersEndWith(const char* text)
{
    const size_t length = strlen(text);
    if (typedLetters.size() < length) {
        return false;
    }
    return typedLetters.compare(typedLetters.size() - length, length, text) == 0;
}

} // namespace

void HandleCheatKey(SDL_Keycode key)
{
    // Codes are letters only; any other key starts over.
    if (key < SDLK_A || key > SDLK_Z) {
        typedLetters.clear();
        return;
    }
    typedLetters.push_back(static_cast<char>(key));
    if (typedLetters.size() > CheatTypedLettersKept) {
        typedLetters.erase(0, typedLetters.size() - CheatTypedLettersKept);
    }
    for (const CheatCode& code : CheatCodes) {
        if (TypedLettersEndWith(code.text)) {
            typedLetters.clear();
            ApplyCheat(code);
            return;
        }
    }
}

} // namespace ckz
