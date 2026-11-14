// Pixels of the desktop key hint badges: a blood splat with the key name or a mouse on top.
// Everything is drawn in code (no image files), so nothing of the game's .big is touched.
#pragma once

#include <string>
#include <vector>

namespace ckz {

// Edge of a square badge in game pixels (the game picture is 1024 x 768).
constexpr int KeyBadgeSize = 64;

/**
 * A badge with `label` (a short key name such as "Q" or "SHIFT") in white with a black outline.
 * `seed` picks the shape of the splat. Returns KeyBadgeSize x KeyBadgeSize RGBA bytes, straight alpha,
 * top row first.
 */
std::vector<unsigned char> MakeKeyBadge(const std::string& label, int seed);

/** A badge with a mouse whose left (or right) button is lit. Same pixel layout as MakeKeyBadge. */
std::vector<unsigned char> MakeMouseBadge(bool rightButton, int seed);

} // namespace ckz
