// Desktop key hints over the mission HUD: at a corner of every HUD control that a key or mouse
// button triggers (KeyBindings.h), a blood splat badge shows that key (pixels: KeyHintArt.cpp).
//
// The badges are a child window put in front of the other HUD children by the CGameplayHUD
// constructor. So they are drawn through the game's own 2D renderer (no GL state of ours), over
// the HUD controls, fade with the controls layer and lie under the pause menu and dialogs.
// [game] KeyHints in zombsniper.cfg turns them off.
#include "Host.h"
#include "Config.h"
#include "KeyHintArt.h"
#include <ctype.h>
#include <stdio.h>
#include <string>
#include <vector>
#include "App.h"
#include "CArrayInputStream.h"
#include "CGameplayHUD.h"
#include "ICRenderSurface.h"
#include "zlib.h"

using namespace com::glu::platform::components;
using namespace com::glu::platform::graphics;

namespace ckz {
namespace {

/** The corner of the control that the badge offset starts from. */
enum class BadgeCorner {
    TopLeft,
    TopRight,
    BottomRight,
    LeftMiddle, // left edge, half way down
};

/** Where the badge of an action sits; Fire is not a GameAction (it is FireButton). */
struct BadgeSpec {
    bool fire;          // the fire button (FireButton); `action` is then not used
    GameAction action;  // the control of this action (Controls.cpp GetActionControl)
    BadgeCorner corner;
    int offsetX;        // from the corner to the badge centre, in game pixels (+ is right / down)
    int offsetY;
};

// Placement of the badges, measured on dumped frames of the survival mission
// (CKZ_DUMP_KEY_HINTS=1 with tests/Mission.KeyHintsLabelHudControls).
const BadgeSpec BadgeSpecs[] = {
    { false, GameAction::Pause, BadgeCorner::BottomRight, -3, -6 },
    { false, GameAction::Aim, BadgeCorner::TopRight, -3, 4 },
    { false, GameAction::UseGrenade, BadgeCorner::TopRight, -3, 4 },
    { false, GameAction::UsePowerUp, BadgeCorner::LeftMiddle, -6, 0 },
    { false, GameAction::SwitchWeapon, BadgeCorner::TopLeft, 2, 4 },
    // The weapon info window (weapon picture and ammo) is wide: the badge goes left of the ammo count.
    { false, GameAction::Reload, BadgeCorner::BottomRight, -134, -22 },
    { true, GameAction::Pause, BadgeCorner::TopLeft, 2, 4 },
};

// Each badge gets its own splat shape from this seed and its index.
constexpr int SplatSeedBase = 17;
constexpr int SplatSeedStep = 7919;
// Mime type key of PNG for ICRenderSurface::Initialize (as CFeaturedAppWindow loads PNG).
constexpr unsigned int PngMimeType = 0xb7178678;

/** Short names for the keys whose SDL names are long. */
struct ShortKeyName {
    SDL_Keycode key;
    const char* name;
};

const ShortKeyName ShortKeyNames[] = {
    { SDLK_LSHIFT, "SHIFT" },
    { SDLK_RSHIFT, "SHIFT" },
    { SDLK_LCTRL, "CTRL" },
    { SDLK_RCTRL, "CTRL" },
    { SDLK_LALT, "ALT" },
    { SDLK_RALT, "ALT" },
    { SDLK_ESCAPE, "ESC" },
    { SDLK_SPACE, "SPACE" },
    { SDLK_RETURN, "ENTER" },
    { SDLK_TAB, "TAB" },
    { SDLK_BACKSPACE, "BKSP" },
};

/** The key or mouse button bound to a control. */
struct BoundInput {
    bool mouse;
    Uint8 button;    // when mouse
    SDL_Keycode key; // otherwise
};

/** The first binding of the badge's control in KeyBindings.h (keys before mouse buttons). */
bool FindInput(const BadgeSpec& spec, BoundInput& input)
{
    if (spec.fire) {
        input.mouse = true;
        input.button = FireButton;
        return true;
    }
    for (const KeyBinding& binding : KeyBindings) {
        if (binding.action == spec.action) {
            input.mouse = false;
            input.key = binding.key;
            return true;
        }
    }
    for (const MouseButtonBinding& binding : MouseButtonBindings) {
        if (binding.action == spec.action) {
            input.mouse = true;
            input.button = binding.button;
            return true;
        }
    }
    return false;
}

/** The name printed on the badge: a short name, else the SDL key name in capitals. */
std::string KeyLabel(SDL_Keycode key)
{
    for (const ShortKeyName& shortName : ShortKeyNames) {
        if (shortName.key == key) {
            return shortName.name;
        }
    }
    std::string name = SDL_GetKeyName(key);
    for (char& letter : name) {
        letter = static_cast<char>(toupper(static_cast<unsigned char>(letter)));
    }
    return name;
}

std::vector<unsigned char> MakeBadgePixels(const BoundInput& input, int seed)
{
    if (!input.mouse) {
        return MakeKeyBadge(KeyLabel(input.key), seed);
    }
    if (input.button == SDL_BUTTON_LEFT) {
        return MakeMouseBadge(false, seed);
    }
    if (input.button == SDL_BUTTON_RIGHT) {
        return MakeMouseBadge(true, seed);
    }
    // Other mouse buttons have no icon: M3, M4, ...
    return MakeKeyBadge("M" + std::to_string(input.button), seed);
}

void AppendBigEndian(std::vector<unsigned char>& bytes, unsigned int value)
{
    bytes.push_back(static_cast<unsigned char>(value >> 24));
    bytes.push_back(static_cast<unsigned char>(value >> 16));
    bytes.push_back(static_cast<unsigned char>(value >> 8));
    bytes.push_back(static_cast<unsigned char>(value));
}

/** Append one PNG chunk: length, type, data and the CRC of type and data. */
void AppendPngChunk(std::vector<unsigned char>& png, const char* type, const std::vector<unsigned char>& data)
{
    AppendBigEndian(png, static_cast<unsigned int>(data.size()));
    const size_t typeStart = png.size();
    png.insert(png.end(), type, type + 4);
    png.insert(png.end(), data.begin(), data.end());
    const uLong crc = crc32(0, &png[typeStart], static_cast<uInt>(png.size() - typeStart));
    AppendBigEndian(png, static_cast<unsigned int>(crc));
}

/**
 * RGBA pixels (top row first) as a PNG file in memory. The engine's hardware surfaces cannot be
 * locked for writing, but they load PNG through its own libpng, as every game image does. The game
 * has no deflate, so the image data goes into uncompressed ("stored") deflate blocks.
 */
std::vector<unsigned char> EncodePng(const std::vector<unsigned char>& pixels, int width, int height)
{
    // Each row starts with filter type 0 (none).
    std::vector<unsigned char> raw;
    const size_t rowBytes = static_cast<size_t>(width) * 4;
    for (int row = 0; row < height; row++) {
        raw.push_back(0);
        raw.insert(raw.end(), pixels.begin() + row * rowBytes, pixels.begin() + (row + 1) * rowBytes);
    }

    // zlib stream: header, stored blocks of at most 65535 bytes, Adler-32 of the raw data.
    constexpr size_t MaxStoredBlock = 65535;
    std::vector<unsigned char> zlibData = { 0x78, 0x01 };
    size_t offset = 0;
    while (offset < raw.size()) {
        size_t blockSize = raw.size() - offset;
        if (blockSize > MaxStoredBlock) {
            blockSize = MaxStoredBlock;
        }
        // Block header byte: bit 0 BFINAL (last block), bits 1-2 BTYPE 00 (stored).
        unsigned char blockHeader = 0;
        if (offset + blockSize == raw.size()) {
            blockHeader = 1;
        }
        zlibData.push_back(blockHeader);
        zlibData.push_back(static_cast<unsigned char>(blockSize));
        zlibData.push_back(static_cast<unsigned char>(blockSize >> 8));
        zlibData.push_back(static_cast<unsigned char>(~blockSize));
        zlibData.push_back(static_cast<unsigned char>(~blockSize >> 8));
        zlibData.insert(zlibData.end(), raw.begin() + offset, raw.begin() + offset + blockSize);
        offset += blockSize;
    }
    AppendBigEndian(zlibData, static_cast<unsigned int>(adler32(1, raw.data(), static_cast<uInt>(raw.size()))));

    // Header: width, height, 8 bits per channel, colour type 6 (RGBA), deflate, no filter set, no interlace.
    std::vector<unsigned char> header;
    AppendBigEndian(header, static_cast<unsigned int>(width));
    AppendBigEndian(header, static_cast<unsigned int>(height));
    header.push_back(8);
    header.push_back(6);
    header.push_back(0);
    header.push_back(0);
    header.push_back(0);

    std::vector<unsigned char> png = { 0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n' };
    AppendPngChunk(png, "IHDR", header);
    AppendPngChunk(png, "IDAT", zlibData);
    AppendPngChunk(png, "IEND", std::vector<unsigned char>());
    return png;
}

/** A surface of the game's 2D renderer holding KeyBadgeSize x KeyBadgeSize RGBA pixels. */
ICRenderSurface* CreateBadgeSurface(const std::vector<unsigned char>& pixels)
{
    const std::vector<unsigned char> png = EncodePng(pixels, KeyBadgeSize, KeyBadgeSize);
    CArrayInputStream stream;
    stream.Open(png.data(), static_cast<unsigned int>(png.size()));
    ICRenderSurface* surface = ICRenderSurface::CreateInstance(ICRenderSurface::Type_Default,
                                                               ICRenderSurface::Targetability_0);
    ICRenderSurface::InitParam params[] = {
        { ICRenderSurface::InitParam_ColorFormat, 0 },
        { ICRenderSurface::InitParam_SourceType, PngMimeType },
        { ICRenderSurface::InitParam_SourceStream, (unsigned int)&stream },
        { ICRenderSurface::InitParam_End, 0 }
    };
    bool initialized = false;
    if (surface != nullptr) {
        initialized = surface->Initialize(params) != 0;
    }
    stream.Close();
    if (!initialized) {
        fprintf(stderr, "Key hints: badge surface could not be created\n");
        delete surface;
        return nullptr;
    }
    return surface;
}

/** The HUD control that `spec` labels. */
Window* GetControl(CGameplayHUD& hud, const BadgeSpec& spec)
{
    if (spec.fire) {
        return hud.m_fireButton;
    }
    switch (spec.action) {
    case GameAction::Reload: return hud.m_weaponInfo;
    case GameAction::SwitchWeapon: return hud.m_switchButton;
    case GameAction::Aim: return hud.m_scopeButton;
    case GameAction::UseGrenade: return hud.m_grenade;
    case GameAction::UsePowerUp: return hud.m_powerUp;
    case GameAction::Pause: return hud.m_pauseButton;
    }
    return nullptr;
}

/** True when `window` and every parent below `hud` are visible (the grenade exists only when owned). */
bool IsShown(Window* window, Window* hud)
{
    if (window == nullptr) {
        return false;
    }
    for (Window* level = window; level != nullptr && level != hud; level = level->m_parent) {
        if ((level->m_flags & Window::FLAG_VISIBLE) == 0) {
            return false;
        }
    }
    return true;
}

/** The point of `control` (global game pixels) named by `corner`. */
void CornerPoint(Window* control, BadgeCorner corner, int& x, int& y)
{
    const int left = control->LocalToGlobalX(0);
    const int top = control->LocalToGlobalY(0);
    const int right = left + control->m_width;
    const int bottom = top + control->m_height;
    switch (corner) {
    case BadgeCorner::TopLeft:
        x = left;
        y = top;
        return;
    case BadgeCorner::TopRight:
        x = right;
        y = top;
        return;
    case BadgeCorner::BottomRight:
        x = right;
        y = bottom;
        return;
    case BadgeCorner::LeftMiddle:
        x = left;
        y = top + control->m_height / 2;
        return;
    }
}

/** Full-screen, input-less child window of the HUD that draws the badges. */
class KeyHintWindow : public Window {
public:
    explicit KeyHintWindow(CGameplayHUD* hud);
    virtual ~KeyHintWindow();
    virtual void Update();
    virtual void Paint(ICGraphics2d& graphics);

    int m_shownCount = 0; // badges drawn by the last Paint (tests)

private:
    struct Badge {
        BadgeSpec spec;
        ICRenderSurface* surface;
    };

    CGameplayHUD* m_hud;
    std::vector<Badge> m_badges;
};

// The hint window of the running HUD (one HUD at a time), for ShownKeyHintCount.
KeyHintWindow* activeHintWindow;

KeyHintWindow::KeyHintWindow(CGameplayHUD* hud)
    : Window(false), m_hud(hud)
{
    // Never take touches or focus: the HUD controls below must get them all. The window keeps the
    // full screen size of a new Window, which it needs to be painted at all (Window::HandlePaint).
    ClearFlags(FLAG_ENABLED | FLAG_FOCUSABLE);
    activeHintWindow = this;
    if (!config.keyHints) {
        ClearFlags(FLAG_VISIBLE);
        return;
    }
    int seed = SplatSeedBase;
    for (const BadgeSpec& spec : BadgeSpecs) {
        seed += SplatSeedStep;
        BoundInput input = {};
        if (!FindInput(spec, input)) {
            continue;
        }
        ICRenderSurface* surface = CreateBadgeSurface(MakeBadgePixels(input, seed));
        if (surface == nullptr) {
            continue;
        }
        Badge badge;
        badge.spec = spec;
        badge.surface = surface;
        m_badges.push_back(badge);
    }
    fprintf(stderr, "Key hints: %d badges\n", static_cast<int>(m_badges.size()));
}

KeyHintWindow::~KeyHintWindow()
{
    for (Badge& badge : m_badges) {
        delete badge.surface;
    }
    if (activeHintWindow == this) {
        activeHintWindow = nullptr;
    }
}

void KeyHintWindow::Update()
{
    Window::Update();
    // Fade with the controls layer (CGameplayHUD::Show dims it while the HUD is hidden).
    SetOpacity(m_hud->m_controlsLayer->m_opacity);
}

void KeyHintWindow::Paint(ICGraphics2d& graphics)
{
    m_shownCount = 0;
    // The controls are local to the HUD; this window may sit elsewhere, so draw relative to it.
    const int originX = LocalToGlobalX(0);
    const int originY = LocalToGlobalY(0);
    for (const Badge& badge : m_badges) {
        Window* control = GetControl(*m_hud, badge.spec);
        if (!IsShown(control, m_hud)) {
            continue;
        }
        int x = 0;
        int y = 0;
        CornerPoint(control, badge.spec.corner, x, y);
        x += badge.spec.offsetX - originX;
        y += badge.spec.offsetY - originY;
        DrawImage(graphics, badge.surface, x, y, ALIGN_HCENTER | ALIGN_VCENTER, 0);
        m_shownCount++;
    }
}

} // namespace

Window* CreateKeyHintWindow(CGameplayHUD* hud)
{
    return new KeyHintWindow(hud);
}

int ShownKeyHintCount()
{
    if (activeHintWindow == nullptr) {
        return 0;
    }
    return activeHintWindow->m_shownCount;
}

} // namespace ckz
