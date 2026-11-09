// App.h - the game application: the window application plus every game manager, the
// gServe connection, timing, device detection and the accelerometer orientation logic.
// Layout from App::App(CNode*), App::InitGameData() and App::Shutdown().
#ifndef APP_H
#define APP_H

#include "WindowApp.h"
#include "TimeValue.h"
#include "CMatrix2d.h"
#include "CVector2d.h"
#include "MathLib.h"
#include "tws.h"
#include <stdlib.h>
#include "CApplet.h"
#include "ICCore.h"
#include "SplashWindow.h"
#include "YesNoDialog.h"
#include "CZombieDialogWindow.h"
#include "CDemoMgr.h"

class CNode;
class AppSettings;
class CDH_SoundManager;
class CHighscoreMgr;
class DGHelper;
class CDH_ResourceManager;
class ResMap;
class CVisualCharacterTypeManager;
class CNationalityTypeManager;
class CEnemyWeaponTypeManager;
class CBH_PersonTypeManager;
class CHuntingInfo;
class CLocationManager;
class CMissionManager;
class CPersonsGroupManager;
class CWaveManager;
class CZombieMissionManager;
class CSceneSetupsManager;
class CBalanceManager;
class CSceneObjectManager;
class CIAPCurrencyManager;
class CEnergyManager;
class CDH_WeaponsManager;
class CSaveManager;
class CTipManager;
class CScriptManager;
class CQuestManager;
class CFiveDaysBonus;
class COffersManager;
class CAchievementManager;
class CCommonGameConfig;
class CSocialGiftManager;
class CNewsFeedManager;
class CProgressSystem;
class CTutorialManager;
class CBH_SurfaceManager;
class SG_Instance;
class CPopupController;
class CUpdateManager;
class CGrenadeManager;
class CGrenadePackManager;
class CPowerUpManager;
class CPowerUpPackManager;
class COverrideManager;
class CGCLeaderboardsManager;
class CNGSLocalUser;
class CProfileManager;  // added by fx2
class CPrivateData1;  // added by fx2
class CNotificationHandler;
class CGServeHandler;
class CNetworkAvailability;
struct lua_State;
class CShopItem;

/** Saved 3D viewport (shorts as the rasterizer state reports them). */
struct SViewport {
    SViewport()
    {
        m_x = 0;
        m_y = 0;
        m_width = 0;
        m_height = 0;
    }

    short m_x;
    short m_y;
    short m_width;
    short m_height;
};

class App : public WindowApp {
public:
    /** Kinds of device (GetDeviceType); DEVICE_UNKNOWN until detected. */
    enum EDeviceType {
        DEVICE_OTHER = 0,
        DEVICE_IPHONE = 2,
        DEVICE_IPHONE_3G = 3,
        DEVICE_IPHONE_3GS = 4,
        DEVICE_IPHONE_4 = 5,
        DEVICE_IPHONE_4_CDMA = 6,
        DEVICE_IPOD_1 = 7,
        DEVICE_IPOD_2 = 8,
        DEVICE_IPOD_3 = 9,
        DEVICE_IPOD_4 = 10,
        DEVICE_IPAD = 11,
        DEVICE_IPAD_2 = 12,
        DEVICE_UNKNOWN = 13
    };

    App(CNode* node);

    inline virtual ~App()
    {
    }

    virtual void Init();
    virtual void Shutdown();
    virtual Window* NewDisplayWindow();
    virtual void Start();
    virtual void OnPause();
    virtual void OnResume();
    virtual void OnEnterBackground();
    virtual void OnEnterForeground();
    virtual void OnBeginWindowPaint(com::glu::platform::graphics::ICGraphics2d& graphics, Window* window);
    virtual void OnEndWindowPaint(com::glu::platform::graphics::ICGraphics2d& graphics, Window* window);
    virtual void OnBeginWindowPaint3D(com::glu::platform::graphics::ICGraphics2d& graphics, Window* window);
    virtual void OnEndWindowPaint3D(com::glu::platform::graphics::ICGraphics2d& graphics, Window* window);
    virtual void OnCommand(Event& event);
    virtual void Update();
    virtual void Paint(com::glu::platform::graphics::ICGraphics2d& graphics);
    virtual void UpdateTime();
    /** Seconds since the application became active. */
    virtual int GetActiveTime() const;

    static bool IsInitialized();
    static int GetCurrentDay();
    bool TimePause();
    bool TimeResume();
    void UpdateTickRate();
    static swerve::Object3D<CSwerveAssertingErrorHandler> LoadObject3D(const char* name);
    static MediaRes MediaCacheRes(const char* name, bool create);
    static void InitNGServer(App* app);
    static void StartNGServer(App* app);
    void InitPlayerIndependentGameData();
    void InitGameData();
    CProgressSystem* InitProgressSystem();
    static void ClearGameData();
    static int GetDeviceType();
    static void OnUpdatesInstalled();
    static void DeleteGameRunningFile();
    static void CreateGameRunningFile();
    void HandleRender();
    static void RenderWorld(const swerve::Graphics3D<CSwerveAssertingErrorHandler>& graphics,
                            const swerve::World<CSwerveAssertingErrorHandler>& world);
    static void Inc2DLayer();
    static bool IsEnoughFreeSpace();
    static CShopItem* GetItemByStringUId(const XString& id);
    static bool IsHD();
    static bool IsVGA();
    static bool IsXGA();
    static bool IsWVGA();
    static bool IsAtrix();
    static bool IsKindle();
    // b145: call-free inline accessor; CAchievementManager::Update reads the tutorial manager through it
    // (it shifts the inliner worklist so the two GameCenter_Facade::GetInstance copies swap stack slots).
    CTutorialManager* GetTutorialManager() const { return m_tutorialManager; }
    // b145: call-free accessor, read by the CGameScene loading step before CNGSLoginFlow::GetInstance.
    CNetworkAvailability* GetNetworkAvailability() const { return m_networkAvailability; }
    // b074i: original 117850 weak body; WVGA keeps 1 first, then selects 2 by HD.
    static int Scale()
    {
        int scale = 1;
        if (!IsWVGA()) {
            if (IsHD()) {
                scale = 2;
            }
        }
        return scale;
    }
    int ConsumeActiveTime();
    static void LoadSGArchetype(int archetype, int character);
    void StartGServerLight();  // changed by b055x (was static; callers pass this in r0)

    static int m_platformType;      // 1
    static int m_deviceType;        // EDeviceType, detected on first use
    static bool m_haveGamecenter;

    void* m_buffer1cc;                          // 0x1cc freed in Shutdown
    void* m_buffer1d0;                          // 0x1d0
    void* m_buffer1d4;                          // 0x1d4
    void* m_buffer1d8;                          // 0x1d8
    void* m_buffer1dc;                          // 0x1dc
    void* m_buffer1e0;                          // 0x1e0
    bool m_initialized;                         // 0x1e4 game data created
    int m_field1e8;                             // 0x1e8
    CNode* m_node;                              // 0x1ec
    float m_timeScale;                          // 0x1f0 0 while paused
    AppSettings* m_settings;                    // 0x1f4
    void* m_field1f8;                           // 0x1f8
    void* m_field1fc;                           // 0x1fc
    void* m_field200;                           // 0x200
    void* m_field204;                           // 0x204
    lua_State* m_lua;                           // 0x208
    CDH_SoundManager* m_soundManager;           // 0x20c
    CHighscoreMgr* m_highscoreManager;          // 0x210
    DGHelper* m_helper;                         // 0x214
    CDH_ResourceManager* m_resourceManager;     // 0x218
    ResMap* m_resMap;                           // 0x21c
    CVisualCharacterTypeManager* m_visualCharacterTypeManager;  // 0x220
    CNationalityTypeManager* m_nationalityTypeManager;          // 0x224
    CEnemyWeaponTypeManager* m_enemyWeaponTypeManager;          // 0x228
    CBH_PersonTypeManager* m_personTypeManager;                 // 0x22c
    void* m_field230;                           // 0x230
    CHuntingInfo* m_huntingInfo;                // 0x234
    CLocationManager* m_locationManager;        // 0x238
    CMissionManager* m_missionManager;          // 0x23c
    CPersonsGroupManager* m_personsGroupManager;    // 0x240
    CWaveManager* m_waveManager;                // 0x244
    CZombieMissionManager* m_zombieMissionManager;  // 0x248
    CSceneSetupsManager* m_sceneSetupsManager;  // 0x24c
    CBalanceManager* m_balanceManager;          // 0x250
    CSceneObjectManager* m_sceneObjectManager;  // 0x254
    CIAPCurrencyManager* m_iapCurrencyManager;  // 0x258
    CEnergyManager* m_energyManager;            // 0x25c
    CDH_WeaponsManager* m_weaponsManager;       // 0x260
    CSaveManager* m_saveManager;                // 0x264
    CTipManager* m_tipManager;                  // 0x268
    CScriptManager* m_scriptManager;            // 0x26c
    CQuestManager* m_questManager;              // 0x270
    CFiveDaysBonus* m_fiveDaysBonus;            // 0x274
    COffersManager* m_offersManager;            // 0x278
    CAchievementManager* m_achievementManager;  // 0x27c
    CCommonGameConfig* m_commonGameConfig;      // 0x280
    CSocialGiftManager* m_socialGiftManager;    // 0x284
    CNewsFeedManager* m_newsFeedManager;        // 0x288
    CProgressSystem* m_progressSystem;          // 0x28c
    CTutorialManager* m_tutorialManager;        // 0x290
    CBH_SurfaceManager* m_surfaceManager;       // 0x294
    SG_Instance* m_sgInstance;                  // 0x298
    CPopupController* m_popupController;        // 0x29c
    CUpdateManager* m_updateManager;            // 0x2a0
    CGrenadeManager* m_grenadeManager;          // 0x2a4
    CGrenadePackManager* m_grenadePackManager;  // 0x2a8
    CPowerUpManager* m_powerUpManager;          // 0x2ac
    CPowerUpPackManager* m_powerUpPackManager;  // 0x2b0
    COverrideManager* m_overrideManager;        // 0x2b4
    CGCLeaderboardsManager* m_leaderboardsManager;  // 0x2b8
    TimeManager m_timeManager;                  // 0x2bc
    // 0x2e8: every reader (CGServeHandler, CNotificationHandler) calls CProfileManager methods on it, but
    // App::InitNGServer stores CNGS::GetLocalUser() there (as in the original binary).  // changed by fx2
    CProfileManager* m_profileManager;          // 0x2e8  // changed by fx2 (was CNGSLocalUser* m_localUser)
    CNotificationHandler* m_notificationHandler;    // 0x2ec
    CGServeHandler* m_gserveHandler;            // 0x2f0
    CPrivateData1* m_privateData;               // 0x2f4 freed in Shutdown  // changed by fx2 (was void* m_buffer2f4)
    void* m_buffer2f8;                          // 0x2f8
    void* m_buffer2fc;                          // 0x2fc
    int m_field300;                             // 0x300
    SViewport m_viewport;                       // 0x304 saved 3D viewport
    com::glu::platform::math::CMatrix2d m_savedTransform;   // 0x30c saved 2D transform
    com::glu::platform::math::CVector2d m_savedTranslation; // 0x324
    float m_orientationChangeTime;              // 0x32c
    int m_pendingOrientation;                   // 0x330 -1: none
    vec3 m_acceleration;                        // 0x334 in screen space
    ImageRes m_images[3];                       // 0x340
    ImageRes m_images2[3];                      // 0x358
    int m_field370;                             // 0x370
    int m_field374;                             // 0x374
    swerve::World<CSwerveAssertingErrorHandler> m_world;    // 0x378
    int m_field37c;                             // 0x37c
    Vector<MediaRes> m_mediaCache;              // 0x380
    int m_timePauseCount;                       // 0x390
    Window* m_mainLayer;                        // 0x394
    Window* m_topLayer;                         // 0x398
    bool m_gameWasRunning;                      // 0x39c gamerun.dat existed at start
    int m_activeStartTime;                      // 0x3a0 seconds
    int m_wantedTickRate;                       // 0x3a4
    int m_tickRate;                             // 0x3a8
    int m_tickRateChangeTime;                   // 0x3ac
    bool m_pushStatisticsSent;                  // 0x3b0
    float m_pushStatisticsDelay;                // 0x3b4 seconds
    CNetworkAvailability* m_networkAvailability;    // 0x3b8
    Window* m_socialConnectDialog;              // 0x3bc social network connect dialog on screen  // changed by fx2 (was int m_field3bc)
    int m_field3c0;                             // 0x3c0
    int m_field3c4;                             // 0x3c4
    bool m_field3c8;                            // 0x3c8
    bool m_field3c9;                            // 0x3c9
};

/** Delays the splash screen by a few frames after the start. */
class StartDelayWindow : public WindowTransparent {
public:
    StartDelayWindow()
        : WindowTransparent(false)
    {
        m_framesLeft = 3;
    }

    virtual ~StartDelayWindow()
    {
    }

    virtual void Update()
    {
        if (m_framesLeft-- > 0) {
            return;
        }
        Close();
        Window* layer = ((App*)WindowApp::m_instance)->m_mainLayer;
        layer->AddModal(new SplashWindow(WindowApp::m_instance->m_restartDelay < 2
            ? SplashWindow::TYPE_1 : SplashWindow::TYPE_2));
    }

    int m_framesLeft;   // 0xac
};

/** Root window of the application: black, handles the global commands. */
class DisplayWindow : public WindowColor {
public:
    /** Global commands. */
    enum {
        COMMAND_SOUND_DIALOG = (int)0x81984c34,
        COMMAND_FORWARD_9B99 = (int)0x9b991bc8,
        COMMAND_UPDATE_GAME = 0x027637f3,
        COMMAND_EXIT = 0x127c5420,
        COMMAND_EXIT_CONSUMED = 0x32ee0670,
        COMMAND_ASK_RESTART = 0x367c4113,
        COMMAND_RESTART = 0x42650932,
        COMMAND_UPGRADE = 0x45ba7539,
        COMMAND_VERSION_CHANGED = 0x6458fa9c
    };

    DisplayWindow()
        : WindowColor(0xff000000)
    {
    }

    virtual ~DisplayWindow()
    {
    }

    virtual void OnCommand(Event& event)
    {
        switch (event.m_id) {
        case COMMAND_SOUND_DIALOG:
            ((App*)WindowApp::m_instance)->m_mainLayer->AddModal(new GameStartSoundDialog());
            event.Clear();
            break;
        case COMMAND_FORWARD_9B99:
            WindowApp::PutEvent(Event::COMMAND, (int)0x97204784, 0, 0, 0);
            event.Clear();
            break;
        case COMMAND_UPDATE_GAME:
            CApplet::m_pCore->LaunchURL(L"http://gcs.glu.com/gcs/iphone/iphone.jsp", 0);
            exit(0);
        case COMMAND_EXIT:
            CApplet::m_pCore->ExitApp((com::glu::platform::framework::ICCore::eExitStatus)0);
            break;
        case COMMAND_EXIT_CONSUMED:
            CApplet::m_pCore->ExitApp((com::glu::platform::framework::ICCore::eExitStatus)0);
            event.Clear();
            break;
        case COMMAND_ASK_RESTART: {
            CZombieDialogWindow* dialog = new CZombieDialogWindow(3);
            dialog->AddButton(CZombieDialogWindow::BUTTON_OK, COMMAND_RESTART, 0, 0, true, 20);
            dialog->SetText(XString("Application will restart"), false, NULL);
            dialog->SetTitle(XString("Restart"));
            AddToFront(dialog);
            ((App*)WindowApp::m_instance)->m_timeScale = 0.0f;
            break;
        }
        case COMMAND_RESTART:
            WindowApp::m_instance->InitializeRestartApplication(1);
            event.Clear();
            break;
        case COMMAND_UPGRADE:
            CDemoMgr::GetInstance()->LaunchUpgrade();
            event.Clear();
            break;
        case COMMAND_VERSION_CHANGED: {
            CZombieDialogWindow* dialog = new CZombieDialogWindow(3);
            dialog->AddButton(CZombieDialogWindow::BUTTON_OK, COMMAND_UPDATE_GAME, 0, 0, true, 20);
            dialog->SetTitle(Window::ResString("IDS_VERSION_CHANGE_TITLE"));
            dialog->SetText(Window::ResString("IDS_VERSION_CHANGE_DESC"), false, NULL);
            AddToFront(dialog);
            ((App*)WindowApp::m_instance)->m_timeScale = 0.0f;
            break;
        }
        }
    }
};

#endif
