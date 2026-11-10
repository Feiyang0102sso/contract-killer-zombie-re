// Bridge SDL lifecycle and pointer events to the original Glu applet.
#include "Host.h"
#include "Config.h"
#include "DebugTools.h"
#include "Log.h"
#ifdef CKZ_TESTS
#include "../../../tests/M3gLoadTest.h"
#include "../../../tests/TestTools.h"
#endif
#include <stdio.h>
#include <wchar.h>
#include <string>
#include <filesystem>
#include "CApplet.h"
#include "CApp.h"
#include "ICCore.h"

using namespace com::glu::platform::framework;

CApplet* CApplet::m_pThis;
ICCore* CApplet::m_pCore;
CApp* CApplet::m_pApp;
short CApplet::m_UIScreenMainScreenWidth = ckz::WindowWidth;
short CApplet::m_UIScreenMainScreenHeight = ckz::WindowHeight;
void* CApplet::m_eaglLayer;

namespace ckz {
SDL_Window* window;
SDL_GLContext context;
const wchar_t* assetPath;
const wchar_t* savePath;
bool running;
}

CApplet::CApplet()
    : m_tickRate(1), m_accelerometerFrequency(0), m_multipleTouchEnabled(0), m_gyroscope(nullptr)
{
    m_pThis = this;
}

CApplet::~CApplet()
{
    Destroy();
    m_pThis = nullptr;
}

unsigned char CApplet::Initialize()
{
    m_pCore = ICCore::CreateInstance();
    m_pApp = CApp::CreateInstance();
    return m_pCore != nullptr && m_pApp != nullptr && m_pApp->Initialize() == 0;
}

void CApplet::Destroy()
{
    if (m_pApp != nullptr) {
        m_pApp->Destroy();
        delete m_pApp;
        m_pApp = nullptr;
    }
    delete m_pCore;
    m_pCore = nullptr;
}

void CApplet::Start() { QueueSystemEvent(SystemEvent_Start, 0, 0); }
void CApplet::Stop()
{
    QueueSystemEvent(SystemEvent_Stop, 0, 0);
    HandleTimer();
}

void CApplet::HandleTimer()
{
    for (int index = 0; index < m_eventQueue.GetCount(); index++) {
        com::glu::platform::core::CSystemEventQueue::SystemEvent event = {};
        m_eventQueue.GetEvent(event, index);
        m_pApp->OnSystemEvent(event.type, event.param1, event.param2);
    }
    m_eventQueue.Clear();
    m_pApp->OnTimer();
}

void CApplet::ExitApp() { ckz::running = false; }
void CApplet::QueueSystemEvent(unsigned int type, unsigned int first, unsigned int second)
{
    m_eventQueue.Queue(type, first, second);
}
void CApplet::SetAccelerometerFrequency(unsigned int frequency) { m_accelerometerFrequency = frequency; }
unsigned char CApplet::IsGyroscopeAvailable() const { return 0; }
void CApplet::SetGyroscopeFrequency(unsigned int) {}

namespace {

using namespace ckz;

/** True when the test build runs without a window (CKZ_HIDDEN); always false in the game build. */
bool IsHiddenRun()
{
#ifdef CKZ_TESTS
    return ckz::test::RunHidden();
#else
    return false;
#endif
}

void QueueTouch(CApplet& applet, unsigned int type, unsigned int id, float x, float y, unsigned int taps)
{
    int width = 0;
    int height = 0;
    SDL_GetWindowSize(ckz::window, &width, &height);
    if (width <= 0 || height <= 0) {
        return;
    }
    // Map from the game picture, which may have black bars around it (GamePictureRect).
    const SDL_Rect picture = GamePictureRect(width, height);
    const float pictureX = (x - picture.x) * ckz::WindowWidth / picture.w;
    const float pictureY = (y - picture.y) * ckz::WindowHeight / picture.h;
    const auto gameX = static_cast<unsigned int>(SDL_clamp(pictureX, 0.0f, ckz::WindowWidth - 1.0f));
    const auto gameY = static_cast<unsigned int>(SDL_clamp(pictureY, 0.0f, ckz::WindowHeight - 1.0f));
    applet.QueueSystemEvent(type, id, ((taps & 3) << 28) | gameX | (gameY << 14));
}

int RunApplet()
{
    CApplet applet;
    if (!applet.Initialize()) {
        fprintf(stderr, "CKZ initialization failed. Check the asset directory and earlier errors.\n");
        return 1;
    }

    ckz::running = true;
    applet.Start();
    // Menus: the cursor is free and the left button is the pointer (touch 1).
    // Mission: the mouse is captured, moving it turns the view and the left button fires.
    bool mouseCaptured = false;
    bool pointerDown = false;
    // Mouse movement collected since the last frame while captured.
    float lookX = 0.0f;
    float lookY = 0.0f;
    while (ckz::running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_EVENT_QUIT:
                ckz::running = false;
                break;
            case SDL_EVENT_WINDOW_FOCUS_LOST:
                if (pointerDown) {
                    applet.QueueSystemEvent(TouchCancelled, 1, 0);
                }
                pointerDown = false;
                SetFireHeld(applet, false);
                break;
            case SDL_EVENT_KEY_DOWN:
                if (event.key.repeat) {
                    break;
                }
#ifdef CKZ_TESTS
                // Test keys for automated checks (tests/TestTools.h).
                if (ckz::test::HandleTestKey(applet, event.key.key, true)) {
                    break;
                }
#endif
                if (event.key.key == SDLK_F9 && ckz::config.debugMode) {
                    // Debug: dump the next game frame.
                    ckz::RequestFrameDump();
                } else {
                    HandleKeyDown(applet, event.key.key);
                    HandleCheatKey(event.key.key);
                }
                break;
            case SDL_EVENT_KEY_UP:
#ifdef CKZ_TESTS
                ckz::test::HandleTestKey(applet, event.key.key, false);
#endif
                break;
            case SDL_EVENT_MOUSE_WHEEL:
                AdjustScopeZoom(event.wheel.y);
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                if (event.button.button != FireButton) {
                    HandleMouseButtonDown(applet, event.button.button);
                } else if (mouseCaptured) {
                    SetFireHeld(applet, true);
                } else if (!SkipMovie()) {
                    // A click during the intro movie only skips it; any other click is a touch.
                    pointerDown = true;
                    QueueTouch(applet, TouchBegan, 1, event.button.x, event.button.y, event.button.clicks);
                }
                break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
                if (event.button.button != FireButton) {
                    break;
                }
                SetFireHeld(applet, false);
                if (pointerDown) {
                    pointerDown = false;
                    QueueTouch(applet, TouchEnded, 1, event.button.x, event.button.y, event.button.clicks);
                }
                break;
            case SDL_EVENT_MOUSE_MOTION:
                if (mouseCaptured) {
                    lookX += event.motion.xrel;
                    lookY += event.motion.yrel;
                } else if (pointerDown) {
                    QueueTouch(applet, TouchMoved, 1, event.motion.x, event.motion.y, 0);
                }
                break;
            }
        }
        if (ckz::running) {
            ApplyDesktopFireMode();
            // Capture the mouse exactly while the mission HUD is live; pause, dialogs and menus free it.
            const bool focused = (SDL_GetWindowFlags(ckz::window) & SDL_WINDOW_INPUT_FOCUS) != 0;
            const bool wantCapture = focused && IsMissionInputActive();
            if (wantCapture != mouseCaptured) {
                mouseCaptured = wantCapture;
                SDL_SetWindowRelativeMouseMode(ckz::window, mouseCaptured);
                fprintf(stderr, "Mouse %s at %llu ms (focus %d)\n", mouseCaptured ? "captured" : "released",
                        static_cast<unsigned long long>(SDL_GetTicks()), focused);
                lookX = 0.0f;
                lookY = 0.0f;
                if (mouseCaptured && pointerDown) {
                    applet.QueueSystemEvent(TouchCancelled, 1, 0);
                    pointerDown = false;
                }
                if (!mouseCaptured) {
                    SetFireHeld(applet, false);
                }
            }
            if (mouseCaptured) {
                TurnView(applet, lookX, lookY);
                lookX = 0.0f;
                lookY = 0.0f;
            }
            applet.HandleTimer();
#ifdef CKZ_TESTS
            ckz::test::UpdateAutoPlay();
            ckz::test::UpdatePickTest();
#endif
            UpdateUnitCountLog();
            SDL_Delay(1);
        }
    }
    applet.Stop();
    return 0;
}

/** Check every entry of RequiredFiles (AssetPaths.h); report each missing file and return false if any. */
bool CheckRequiredFiles(const std::filesystem::path& assetsFolder, const std::filesystem::path& executableFolder)
{
    std::string report;
    for (const RequiredFile& file : RequiredFiles) {
        std::filesystem::path folder = assetsFolder;
        if (file.location == FileLocation::ExecutableFolder) {
            folder = executableFolder;
        }
        const std::filesystem::path path = folder / file.name;
        if (std::filesystem::is_regular_file(path)) {
            continue;
        }
        const std::string line = "Missing file: " + path.u8string() + " (" + file.purpose + ")\n";
        fputs(line.c_str(), stderr);
        report += line;
    }
    if (report.empty()) {
        return true;
    }
    // The game has no console unless DebugMode is on, so also say it in a dialog.
    if (!IsHiddenRun()) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, ckz::config.title.c_str(), report.c_str(), nullptr);
    }
    return false;
}

/** Create the game window (WindowWidth / WindowHeight / FullScreen / Title of the config) and its GL context. */
bool CreateGameWindow()
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_WindowFlags windowFlags = SDL_WINDOW_OPENGL;
    if (IsHiddenRun()) {
        windowFlags |= SDL_WINDOW_HIDDEN;
    }
    int windowWidth = ckz::config.windowWidth;
    int windowHeight = ckz::config.windowHeight;
    if (ckz::config.fullScreen) {
        // Borderless full screen at the desktop resolution; the window size is used when leaving it.
        windowFlags |= SDL_WINDOW_FULLSCREEN;
        fprintf(stderr, "Window: full screen\n");
    } else {
        // Use the configured window size, or the largest one of the same shape that fits the desktop.
        SDL_Rect desktop;
        if (SDL_GetDisplayUsableBounds(SDL_GetPrimaryDisplay(), &desktop)) {
            const int freeHeight = desktop.h - ckz::TitleBarAllowance;
            if (windowWidth > desktop.w || windowHeight > freeHeight) {
                const float fitX = static_cast<float>(desktop.w) / windowWidth;
                const float fitY = static_cast<float>(freeHeight) / windowHeight;
                const float fit = SDL_min(fitX, fitY);
                windowWidth = static_cast<int>(windowWidth * fit);
                windowHeight = static_cast<int>(windowHeight * fit);
            }
            fprintf(stderr, "Window %dx%d (wanted %dx%d, desktop %dx%d)\n", windowWidth, windowHeight,
                    ckz::config.windowWidth, ckz::config.windowHeight, desktop.w, desktop.h);
        }
    }
    ckz::window = SDL_CreateWindow(ckz::config.title.c_str(), windowWidth, windowHeight, windowFlags);
    if (ckz::window != nullptr) {
        if (!ckz::config.fullScreen) {
            // SDL centres the picture, which can push the title bar of a tall window above the screen.
            int titleBarHeight = 0;
            int windowX = 0;
            int windowY = 0;
            SDL_GetWindowBordersSize(ckz::window, &titleBarHeight, nullptr, nullptr, nullptr);
            SDL_GetWindowPosition(ckz::window, &windowX, &windowY);
            if (windowY - titleBarHeight < 0) {
                SDL_SetWindowPosition(ckz::window, windowX, titleBarHeight);
            }
        }
        ckz::context = SDL_GL_CreateContext(ckz::window);
    }
    if (ckz::context == nullptr) {
        fprintf(stderr, "Could not create the OpenGL window: %s\n", SDL_GetError());
        SDL_DestroyWindow(ckz::window);
        return false;
    }
    // VSync=0 shows the frame rate the game can reach without waiting for the display.
    if (ckz::config.vSync) {
        SDL_GL_SetSwapInterval(1);
    } else {
        SDL_GL_SetSwapInterval(0);
    }
    ckz::ConfigureDesktopGl();
    return true;
}

/** Everything after the log and the config: check the files, open the window, run the game. */
int RunGame(int argc, wchar_t** argv, const std::filesystem::path& binaryPath)
{
    namespace fs = std::filesystem;
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        return 1;
    }
    fs::path assets = binaryPath / ckz::AssetsFolderName;
    fs::path saves = binaryPath / ckz::SavesFolderName;
    if (argc >= 2) {
        assets = fs::absolute(argv[1]);
    }
    if (argc >= 3) {
        saves = fs::absolute(argv[2]);
    }
    const std::wstring assetsString = assets.generic_wstring();
    const std::wstring savesString = saves.generic_wstring();
    ckz::assetPath = assetsString.c_str();
    ckz::savePath = savesString.c_str();
    if (!CheckRequiredFiles(assets, binaryPath)) {
        SDL_Quit();
        return 1;
    }
    fs::create_directories(saves);
    if (!CreateGameWindow()) {
        SDL_Quit();
        return 1;
    }
    int result = 0;
#ifdef CKZ_TESTS
    const char* m3gTestFolder = SDL_getenv(ckz::m3g::LoadTestVariable);
    if (m3gTestFolder != nullptr) {
        // Native M3G engine check (PORT_PLAN.md, P2 / M0) instead of the game.
        result = ckz::m3g::RunLoadTest(m3gTestFolder);
    } else {
        result = RunApplet();
    }
#else
    result = RunApplet();
#endif
    SDL_GL_DestroyContext(ckz::context);
    SDL_DestroyWindow(ckz::window);
    SDL_Quit();
    return result;
}

} // namespace

int wmain(int argc, wchar_t** argv)
{
    // The log comes first so that it holds every line, including the config's own messages.
    ckz::StartLog();
    const std::filesystem::path binaryPath = ckz::ExecutableFolder();
    ckz::LoadConfig(binaryPath / ckz::ConfigFileName);
    if (ckz::config.debugMode && !IsHiddenRun()) {
        ckz::OpenLogConsole();
    }
    const int result = RunGame(argc, argv, binaryPath);
    fprintf(stderr, "Exit code %d\n", result);
    ckz::StopLogConsole();
    return result;
}
