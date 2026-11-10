// Cheat codes: type the letters on the keyboard at any time once the game has loaded.
// This file holds the code texts and their numbers; Cheat.cpp applies them.
#pragma once

#include <SDL3/SDL.h>

namespace ckz {

/** What a cheat code does; `amount` of the code says how much. */
enum class CheatEffect {
    AddLevels,     // raise the player level by `amount`
    AddCurrencies, // add `amount` to both the soft currency (cash) and the hard currency (credits)
};

struct CheatCode {
    const char* text; // lower-case letters only
    CheatEffect effect;
    int amount;
};

constexpr int CheatCurrencyAmount = 9999999;

// A code fires as soon as the last typed letters spell it, so no code may end with another code.
constexpr CheatCode CheatCodes[] = {
    { "chxp", CheatEffect::AddLevels, 1 },
    { "chxxp", CheatEffect::AddLevels, 10 },
    { "chm", CheatEffect::AddCurrencies, CheatCurrencyAmount },
};

// How many of the last typed letters are remembered; must be at least the longest code.
constexpr size_t CheatTypedLettersKept = 16;

/** Feed one pressed key; applies a cheat when the typed letters complete its code. */
void HandleCheatKey(SDL_Keycode key);

// Debug: with CKZ_AUTOPLAY=survival the survival mission starts as soon as the map is open
// (automated performance and rendering checks, see PORT_PLAN.md P2).
constexpr const char* AutoPlayVariable = "CKZ_AUTOPLAY";
constexpr const char* AutoPlaySurvival = "survival";

/** Call once per frame; starts the auto play mission once. */
void UpdateAutoPlay();

} // namespace ckz
