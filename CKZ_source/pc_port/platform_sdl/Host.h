#pragma once

#include <SDL3/SDL.h>
#include "AssetPaths.h"
#include "Cheat.h"
#include "KeyBindings.h"

class CApplet;

namespace ckz {

// Resolution the game renders and lays out at: the only iPad size its assets (vga.big) support.
constexpr int WindowWidth = 1024;
constexpr int WindowHeight = 768;
// The desktop window size is WindowWidth / WindowHeight of zombsniper.cfg (Config.h); its default
// is the iPad 4 screen, which showed the same 1024 x 768 game at double size.
// Desktop height kept free for the window's title bar when fitting the window.
constexpr int TitleBarAllowance = 72;
constexpr unsigned int TouchBegan = 0x35c43e56;
constexpr unsigned int TouchMoved = 0x35cb8f1c;
constexpr unsigned int TouchEnded = 0x35c38d1c;
constexpr unsigned int TouchCancelled = 0x5707b968;

extern SDL_Window* window;
extern SDL_GLContext context;
extern const wchar_t* assetPath;
extern const wchar_t* savePath;
extern bool running;
void ConfigureDesktopGl();
/**
 * Where the game picture sits in a window of this size: as large as fits without changing its
 * shape, centred; the rest stays black. `x` / `y` count from the window's top-left corner.
 */
SDL_Rect GamePictureRect(int windowWidth, int windowHeight);
/** While a movie plays, blit its frame into `picture` of the window (draw framebuffer 0, GL origin
 *  bottom-left) and return true. */
bool BlitMovieFrame(const SDL_Rect& picture);
/** Stop the playing movie, like a touch did on iOS. Returns true when a movie was skipped. */
bool SkipMovie();
/** Tap the HUD control of the action; returns false when it is not available right now. */
bool TriggerAction(CApplet& applet, GameAction action);
/** Look the input up in KeyBindings.h and trigger its action. */
void HandleKeyDown(CApplet& applet, SDL_Keycode key);
/** Returns true when the button is bound to an action (and so is not the pointer). */
bool HandleMouseButtonDown(CApplet& applet, Uint8 button);
/** Zoom the scope of a sniper rifle in (wheelY > 0) or out; does nothing unless scoped. */
void AdjustScopeZoom(float wheelY);
/** Keep the game in its HUD fire button mode: the fire button is what the left mouse button holds. */
void ApplyDesktopFireMode();
/** True while the mission HUD takes input; the mouse is captured exactly then. */
bool IsMissionInputActive();
/** Turn the view by a mouse movement (in mouse counts), as a finger drag would. */
void TurnView(CApplet& applet, float mouseX, float mouseY);
/** Press or release the HUD fire button. */
void SetFireHeld(CApplet& applet, bool held);
/** Debug (F9): write the next presented game frame to frame_<n>_<w>x<h>.rgba next to the executable. */
void RequestFrameDump();

} // namespace ckz
