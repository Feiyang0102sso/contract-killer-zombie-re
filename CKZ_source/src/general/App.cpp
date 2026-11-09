// App.cpp - the game application: creates and destroys every game manager, connects to
// gServe, keeps the timing (tick rate, pause), detects the device and turns the
// accelerometer into screen orientation changes.
#include "App.h"
#include <math.h>
#include <string.h>
#include <wchar.h>
#include <sys/types.h>
#ifndef CKZ_PC
#include <sys/sysctl.h>
#endif
#include "CNode.h"
#include "AppSettings.h"
#include "LanguageDialog.h"
#include "CStdUtil_Cocoa.h"
#include "CStrChar.h"
#include "CStrWChar.h"
#include "CAppInfo.h"
#include "CFileUtil.h"
#include "ICFileMgr.h"
#include "ICFile.h"
#include "ICStdUtil.h"
#include "CStringToKey.h"
#include "ICGraphics.h"
#include "ICGraphics2d.h"
#include "ICRasterizerState.h"
#include "AdBannerSizes.h"
#include "CNGSOfferManager.h"
#include "CNGS.h"
#include "CNGSURLMgr.h"
#include "CNGSFactory.h"
#include "CNGSLocalUser.h"
#include "CNGSLoginFlow.h"
#include "CNetMessageQueue_gServe.h"
#include "CNetworkAvailability.h"
#include "ICInAppPurchase.h"
#include "LocalPushManager.h"
#include "CFlurry.h"
#include "TapjoyInterface.h"
#include "SG_Home.h"
#include "SG_Instance.h"
#include "SwerveHelper.h"
#include "CGameApp.h"
#include "CUtility.h"
#include "CNotificationHandler.h"
#include "CGServeHandler.h"
#include "CLoginSteps.h"
#include "CBountyGameFactory.h"
#include "CBH_Player.h"
#include "CBH_SurfaceManager.h"
#include "CGameAnalytics.h"
#include "CDH_SoundManager.h"
#include "CDH_ResourceManager.h"
#include "CDH_WeaponsManager.h"
#include "ResMap.h"
#include "CHighscoreMgr.h"
#include "DGHelper.h"
#include "COverrideManager.h"
#include "CBalanceManager.h"
#include "CVisualCharacterTypeManager.h"
#include "CScriptManager.h"
#include "CCommonGameConfig.h"
#include "CFiveDaysBonus.h"
#include "CAchievementManager.h"
#include "CNationalityTypeManager.h"
#include "CEnemyWeaponTypeManager.h"
#include "CBH_PersonTypeManager.h"
#include "CHuntingInfo.h"
#include "CSceneObjectManager.h"
#include "CLocationManager.h"
#include "CPersonsGroupManager.h"
#include "CWaveManager.h"
#include "CZombieMissionManager.h"
#include "CSceneSetupsManager.h"
#include "CMissionManager.h"
#include "CQuestManager.h"
#include "CIAPCurrencyManager.h"
#include "CGCLeaderboardsManager.h"
#include "CGrenadeManager.h"
#include "CShopPackManager.h"
#include "CPowerUpManager.h"
#include "CTipManager.h"
#include "COffersManager.h"
#include "CTutorialManager.h"
#include "CPopupController.h"
#include "CSocialGiftsManager.h"
#include "CProgressSystem.h"
#include "CNewsFeedManager.h"
#include "CEnergyManager.h"
#include "CUpdateManager.h"
#include "CTjManager.h"
#include "CSaveManager.h"

using namespace com::glu::platform;
using namespace com::glu::platform::components;
using namespace com::glu::platform::graphics;

// Lua runtime (C library, no header in the project yet)
extern "C" {
lua_State* luaL_newstate();
void luaL_openlibs(lua_State* state);
void lua_close(lua_State* state);
}

static const char GSERV_URL[] = "http://ng-zombsniper.glu.com/zombsniper/";
static const char FLURRY_API_KEY[] = "EEKPX1UWQSTW45ZKLVJL";
static const wchar_t gameRunningFilename[] = L"gamerun.dat";

/** Tick rates (ms per frame) while playing and while idle. */
static const int TICK_RATE_ACTIVE = 30;
static const int TICK_RATE_IDLE = 5;
/** A frame longer than this (ms) keeps the active tick rate. */
static const int SLOW_FRAME_TIME = 110;
/** Delay before the tick rate is lowered (ms). */
static const int TICK_RATE_DELAY = 1000;
/** Longest frame used for the game clock (ms). */
static const int MAX_FRAME_TIME = 200;
/** Delay before the push token statistics are sent (seconds). */
static const float PUSH_STATISTICS_DELAY = 5.0f;
/** gServe settings. */
static const int SERVER_ENVIRONMENT = 3;
static const unsigned int GSERV_GAME_ID = 0x1e1b620a;
/** Free disk space needed to save (bytes). */
static const unsigned int MIN_FREE_SPACE = 1024;
/** Accelerometer orientation detection. */
static const float FACE_DOWN_Z = -0.8f;
static const float ORIENTATION_MIN_DIFFERENCE = 0.2f;
static const float ORIENTATION_DELAY = 0.8f;
/** Commands handled by App::OnCommand. */
static const int COMMAND_ACCELEROMETER = (int)0xad174cef;
static const int COMMAND_TAPJOY = (int)0xd519d14c;
static const int COMMAND_SELECT_LANGUAGE = 0x79ea2236;
/** Sounds paused while the application is in the background. */
static const CDH_SoundManager::eSound BACKGROUND_SOUNDS = CDH_SoundManager::SOUND_MUSIC_ALL;

int App::m_platformType = 1;
int App::m_deviceType = DEVICE_UNKNOWN;
bool App::m_haveGamecenter;

App::App(CNode* node)
    : m_initialized(false)
{
    m_field1e8 = 0;
    m_node = node;
    m_timeScale = 1.0f;
    m_commonGameConfig = NULL;
    m_settings = NULL;
    m_field1f8 = NULL;
    m_field1fc = NULL;
    m_field200 = NULL;
    m_field204 = NULL;
    m_lua = NULL;
    m_soundManager = NULL;
    m_highscoreManager = NULL;
    m_helper = NULL;
    m_resourceManager = NULL;
    m_resMap = NULL;
    m_visualCharacterTypeManager = NULL;
    m_nationalityTypeManager = NULL;
    m_enemyWeaponTypeManager = NULL;
    m_personTypeManager = NULL;
    m_field230 = NULL;
    m_huntingInfo = NULL;
    m_locationManager = NULL;
    m_missionManager = NULL;
    m_personsGroupManager = NULL;
    m_waveManager = NULL;
    m_zombieMissionManager = NULL;
    m_sceneSetupsManager = NULL;
    m_balanceManager = NULL;
    m_sceneObjectManager = NULL;
    m_iapCurrencyManager = NULL;
    m_energyManager = NULL;
    m_weaponsManager = NULL;
    m_saveManager = NULL;
    m_tipManager = NULL;
    m_scriptManager = NULL;
    m_questManager = NULL;
    m_progressSystem = NULL;
    m_tutorialManager = NULL;
    m_surfaceManager = NULL;
    m_sgInstance = NULL;
    m_popupController = NULL;
    m_updateManager = NULL;
    m_grenadeManager = NULL;
    m_grenadePackManager = NULL;
    m_powerUpManager = NULL;
    m_powerUpPackManager = NULL;
    m_overrideManager = NULL;
    m_leaderboardsManager = NULL;
    m_haveGamecenter = false;
    m_activeStartTime = CStdUtil_Cocoa::GetTimeSeconds();
    m_privateData = NULL;  // changed by fx2
    m_buffer1d4 = NULL;
    m_buffer1cc = NULL;
    m_notificationHandler = NULL;
    m_gserveHandler = NULL;
    m_buffer2f8 = NULL;
    m_buffer1d8 = NULL;
    m_buffer1d0 = NULL;
    m_newsFeedManager = NULL;
    m_buffer2fc = NULL;
    m_buffer1e0 = NULL;
    m_buffer1dc = NULL;
    m_socialConnectDialog = NULL;  // changed by fx2
    m_field3c0 = -1;
    m_field3c4 = 0;
    m_field3c8 = false;
    m_field300 = 0;
    m_pushStatisticsSent = false;
    m_pushStatisticsDelay = PUSH_STATISTICS_DELAY;
    m_orientationChangeTime = 0.0f;
    m_pendingOrientation = -1;
    m_acceleration.m_x = 0.0f;
    m_acceleration.m_y = 0.0f;
    m_acceleration.m_z = 0.0f;
    m_field370 = 0;
    m_field374 = 0;
    m_field37c = 0;
    m_gameWasRunning = false;
    m_tickRateChangeTime = 0;
    m_tickRate = TICK_RATE_ACTIVE;
    m_wantedTickRate = TICK_RATE_ACTIVE;
    CApplet::m_pCore->SetTickRate(TICK_RATE_ACTIVE);
    // Game Center needs iOS 4.2
    CStrWChar version = CApplet::m_pCore->GetPlatformVersion();
    if (wcscmp(version.m_string, L"4.2") < 0) {
        m_haveGamecenter = false;
    }
    m_timePauseCount = 0;
    m_field3c9 = false;
}

bool App::IsInitialized()
{
    bool initialized = false;
    if (WindowApp::m_instance != NULL) {
        initialized = ((App*)WindowApp::m_instance)->m_initialized;
    }
    return initialized;
}

int App::GetCurrentDay()
{
    int seconds = CStdUtil_Cocoa::GetTimeSeconds();
    if (seconds == -1) {
        return -1;
    }
    return DGHelper::GetDaysFromTimestamp(seconds);
}

bool App::TimePause()
{
    // Preserve the original 32-bit wrap before the signed comparison.
    int count = static_cast<int>(static_cast<unsigned int>(m_timePauseCount) + 1U);
    m_timePauseCount = count;
    if (count < 1) {
        return false;
    }
    ((App*)WindowApp::m_instance)->m_timeScale = 0.0f;
    return true;
}

bool App::TimeResume()
{
    m_timePauseCount--;
    if (m_timePauseCount > 0) {
        return false;
    }
    ((App*)WindowApp::m_instance)->m_timeScale = 1.0f;
    return true;
}

void App::UpdateTickRate()
{
    App* app = (App*)WindowApp::m_instance;
    bool active = true;
    if (app->m_field1e8 == 0) {
        active = app->m_updateFrameTime > SLOW_FRAME_TIME;
    }
    int rate = TICK_RATE_IDLE;
    if (active) {
        rate = TICK_RATE_ACTIVE;
    }
    if (rate != m_wantedTickRate) {
        m_tickRateChangeTime = app->m_updateTime;
        m_wantedTickRate = rate;
        // speeding up happens at once
        if (active && m_tickRate != rate) {
            m_tickRate = rate;
            CApplet::m_pCore->SetTickRate(rate);
        }
    } else if (m_tickRate != m_wantedTickRate) {
        // slowing down waits a second
        if (app->m_updateTime - m_tickRateChangeTime > TICK_RATE_DELAY) {
            m_tickRate = m_wantedTickRate;
            CApplet::m_pCore->SetTickRate(m_wantedTickRate);
        }
    }
}

swerve::Object3D<CSwerveAssertingErrorHandler> App::LoadObject3D(const char* name)
{
    swerve::Object3D<CSwerveAssertingErrorHandler> object;
    XString fileName = ((App*)WindowApp::m_instance)->m_resMap->getFilename(name);
    if (fileName.Length() > 0) {
        object = SwerveHelper::LoadObject3D(fileName.c_str(), 0);
    }
    if (object.m_interface == NULL) {
        object = SwerveHelper::LoadObject3D(name);
    }
    return object;
}

MediaRes App::MediaCacheRes(const char* name, bool create)
{
    Vector<MediaRes>& cache = ((App*)WindowApp::m_instance)->m_mediaCache;
    for (int index = 0; index < cache.size(); index++) {
        MediaRes& cached = cache[index];
        if (strcmp(cached.GetResourceId(), name) == 0) {
            return cached;
        }
    }
    if (!create) {
        return MediaRes();
    }
    MediaRes resource(name);
    if (resource.m_resource != NULL) {
        cache.addElement(resource);
    }
    return resource;
}

void App::InitNGServer(App* app)
{
    CNGS* ngs = CNGS::GetInstance();
    CNGSURLMgr::GetInstance()->setServerEnvironment((eServerEnvType)SERVER_ENVIRONMENT);
    CNetMessageQueue_gServe::GetInstance()->onStart();
    ngs->SetServerURL(GSERV_URL);
    ngs->m_gameId = GSERV_GAME_ID;
    ngs->SetFactory(new CBountyGameFactory());
    wchar_t number[8];
    core::ICStdUtil::SWPrintF(number, L"%d", CAppInfo::GetInstance()->GetVersionMajor());
    CStrWChar major(number);
    core::ICStdUtil::SWPrintF(number, L"%d", CAppInfo::GetInstance()->GetVersionMinor());
    CStrWChar minor(number);
    core::ICStdUtil::SWPrintF(number, L"%d", CAppInfo::GetInstance()->GetVersionMicro());
    CStrWChar micro(number);
    ngs->setVersion(major, minor, micro);
    // The original stores the local user in the field its readers use as the profile manager.  // changed by fx2
    app->m_profileManager = (CProfileManager*)CNGS::GetInstance()->GetLocalUser();  // changed by fx2
    app->m_notificationHandler = new CNotificationHandler();
    CNGS::GetInstance()->GetLocalUser()->registerNotificationHandler(app->m_notificationHandler);
    if (app->m_gserveHandler == NULL) {
        CGServeHandler* handler = new CGServeHandler();
        app->m_gserveHandler = handler;
        handler->Init();
    }
}

void App::StartNGServer(App* app)
{
}

void App::InitPlayerIndependentGameData()
{
    m_newsFeedManager = new CNewsFeedManager();
}

void App::InitGameData()
{
    m_overrideManager = new COverrideManager();
    m_balanceManager = new CBalanceManager();
    m_visualCharacterTypeManager = new CVisualCharacterTypeManager();
    m_scriptManager = new CScriptManager();
    m_commonGameConfig = new CCommonGameConfig();
    m_fiveDaysBonus = new CFiveDaysBonus();
    m_achievementManager = new CAchievementManager();
    m_nationalityTypeManager = new CNationalityTypeManager();
    m_enemyWeaponTypeManager = new CEnemyWeaponTypeManager();
    m_personTypeManager = new CBH_PersonTypeManager();
    m_huntingInfo = new CHuntingInfo();
    m_sceneObjectManager = new CSceneObjectManager();
    m_locationManager = new CLocationManager();
    m_personsGroupManager = new CPersonsGroupManager();
    m_waveManager = new CWaveManager();
    m_zombieMissionManager = new CZombieMissionManager();
    m_sceneSetupsManager = new CSceneSetupsManager();
    m_missionManager = new CMissionManager();
    m_questManager = new CQuestManager();
    m_iapCurrencyManager = new CIAPCurrencyManager();
    m_leaderboardsManager = new CGCLeaderboardsManager();
    m_weaponsManager = new CDH_WeaponsManager();
    InitProgressSystem();
    m_weaponsManager->Init();
    m_grenadeManager = new CGrenadeManager();
    m_grenadePackManager = new CGrenadePackManager();
    m_powerUpManager = new CPowerUpManager();
    m_powerUpPackManager = new CPowerUpPackManager();
    m_tipManager = new CTipManager();
    m_offersManager = new COffersManager();
    m_tutorialManager = new CTutorialManager();
    ((App*)WindowApp::m_instance)->m_mainLayer->AddToFront(m_tutorialManager);
    m_tutorialManager->ClearFlags(Window::FLAG_FOCUSABLE);
    m_popupController = new CPopupController();
    ((App*)WindowApp::m_instance)->m_mainLayer->AddToFront(m_popupController);
    m_socialGiftManager = new CSocialGiftManager();
    ICInAppPurchase::GetInstance()->Init(CGameApp::GetInstance());
    ((App*)WindowApp::m_instance)->m_iapCurrencyManager->UpdateIAPInfo(false);
    m_initialized = true;
}

CProgressSystem* App::InitProgressSystem()
{
    CProgressSystem* progressSystem = m_progressSystem;
    if (progressSystem == NULL) {
        progressSystem = new CProgressSystem();
        m_progressSystem = progressSystem;
    }
    return progressSystem;
}

void App::ClearGameData()
{
    ((App*)WindowApp::m_instance)->m_initialized = false;
    CBH_Player* player = CBH_Player::GetInstance();
    if (player != NULL) {
        delete player;
    }
    CTjManager* tapjoy = CTjManager::GetInstance();
    if (tapjoy != NULL) {
        delete tapjoy;
    }
    CGameApp::GetInstance()->ReInit();
    ICInAppPurchase* purchase = ICInAppPurchase::GetInstance();
    if (purchase != NULL) {
        delete purchase;
    }
    ICInAppPurchase::GetInstance()->Init(CGameApp::GetInstance());
}

void App::Init()
{
    GetDeviceType();
    CApplet::m_pCore->SetBacklightEnabled(0);
    core::CStringToKey("EventId_LaunchVIPLink", 0);
    CGameAnalytics::logAppStart();
    m_lua = luaL_newstate();
    luaL_openlibs(m_lua);
    SG_Home::GetInstance()->Init();
    m_settings = new AppSettings();
    m_surfaceManager = new CBH_SurfaceManager();
    m_networkAvailability = new CNetworkAvailability();
    InitNGServer(this);
    m_updateManager = new CUpdateManager();
    m_updateManager->init();
    m_updateManager->m_onInstalled = OnUpdatesInstalled;
    m_updateManager->checkUpdates();
    if (m_updateManager->hasCompleteUpdates() == 1) {
        m_updateManager->installUpdates(true);
    }
    m_saveManager = new CSaveManager();
    m_saveManager->Init();
    m_resMap = new ResMap();
    m_resMap->init();
    m_resourceManager = new CDH_ResourceManager();
    m_soundManager = new CDH_SoundManager();
    m_soundManager->SetActivated(true);
    m_highscoreManager = new CHighscoreMgr();
    m_helper = new DGHelper();
    MediaCacheRes("IDM_WINDOW_FLYOUT", true);
    MediaCacheRes("IDM_ITEM_SELECTED", true);
    MediaCacheRes("IDM_MENU_CLICK", true);
    int screenWidth = WindowApp::m_instance->m_screenWidth;
    int screenSize = WindowApp::m_instance->m_screenHeight;
    if (screenWidth > screenSize) {
        screenSize = screenWidth;
    }
    m_field370 = screenSize / 7;
    m_field374 = Window::ImageHeight(m_images[1].GetSurface());
    // gamerun.dat exists while the game runs: finding it means the last run did not end normally
    {
        CStrWChar path;
        CFileUtil::GetApplicationDataPathForFile(path, gameRunningFilename);
        ICFileMgr* files = ICFileMgr::GetInstance();
        m_gameWasRunning = files->Exists(path.m_string) != 0;
        files->Delete(path.m_string);
    }
    CFlurry::StartSession(FLURRY_API_KEY);
    CFlurry::SetSessionReportsOnPauseEnabled(true);
    CFlurry::SetSessionReportsOnCloseEnabled(false);
    CStrWChar platform = CApplet::m_pCore->GetPlatformIdentifier();
    wcscmp(platform.m_string, L"iPad1,1");
    CGameAnalytics::SendDeferredPushToken();
    CUtility::RegisterUIKeys();
    LocalPushManager::CancelLocalPushNotifications(false);
}

// sysctlbyname fills a buffer of the C library allocator
#undef malloc
#undef free
extern "C" void* malloc(size_t size);
extern "C" void free(void* pointer);
int App::GetDeviceType()
{
    if (m_deviceType == DEVICE_UNKNOWN) {
        CStrChar machine;
#ifdef CKZ_PC
        // PORT: Use the iPad 2 resource and gameplay capability profile at 1024 x 768.
        machine = "iPad2,1";
#else
        size_t size;
        sysctlbyname("hw.machine", NULL, &size, NULL, 0);
        char* buffer = (char*)malloc(size);
        sysctlbyname("hw.machine", buffer, &size, NULL, 0);
        machine = buffer;
        free(buffer);
#endif
        const char* model = machine.m_string;
        if (model == NULL) {
            m_deviceType = DEVICE_OTHER;
        } else if (strcmp(model, "iPhone1,1") == 0) {
            m_deviceType = DEVICE_IPHONE;
        } else if (strcmp(model, "iPhone1,2") == 0) {
            m_deviceType = DEVICE_IPHONE_3G;
        } else if (strcmp(model, "iPhone2,1") == 0) {
            m_deviceType = DEVICE_IPHONE_3GS;
        } else if (strcmp(model, "iPhone3,1") == 0) {
            m_deviceType = DEVICE_IPHONE_4;
        } else if (strcmp(model, "iPhone3,3") == 0) {
            m_deviceType = DEVICE_IPHONE_4_CDMA;
        } else if (strcmp(model, "iPod1,1") == 0) {
            m_deviceType = DEVICE_IPOD_1;
        } else if (strcmp(model, "iPod2,1") == 0) {
            m_deviceType = DEVICE_IPOD_2;
        } else if (strcmp(model, "iPod3,1") == 0) {
            m_deviceType = DEVICE_IPOD_3;
        } else if (strcmp(model, "iPod4,1") == 0) {
            m_deviceType = DEVICE_IPOD_4;
        } else if (strcmp(model, "iPad1,1") == 0) {
            m_deviceType = DEVICE_IPAD;
        } else if (strcmp(model, "iPad2,1") == 0) {
            m_deviceType = DEVICE_IPAD_2;
        } else if (strcmp(model, "iPad2,2") == 0) {
            m_deviceType = DEVICE_IPAD_2;
        } else if (strcmp(model, "iPad2,3") == 0) {
            m_deviceType = DEVICE_IPAD_2;
        } else if (strcmp(model, "i386") == 0) {
            // simulator: guess from the screen size
            int screenWidth = WindowApp::m_instance->m_screenWidth;
            int screenSize = WindowApp::m_instance->m_screenHeight;
            if (screenWidth > screenSize) {
                screenSize = screenWidth;
            }
            if (screenSize <= 480) {
                m_deviceType = DEVICE_IPHONE_3G;
            } else if (screenSize <= 960) {
                m_deviceType = DEVICE_IPHONE_4;
            } else if (screenSize <= 1024) {
                m_deviceType = DEVICE_IPAD;
            }
        } else {
            m_deviceType = DEVICE_OTHER;
        }
    }
    return m_deviceType;
}
#define malloc np_malloc
#define free np_free

void App::OnUpdatesInstalled()
{
    App* app = (App*)WindowApp::m_instance;
    if (app->m_offersManager != NULL) {
        app->m_offersManager->ReloadConfig();
        app = (App*)WindowApp::m_instance;
    }
    if (app->m_overrideManager != NULL) {
        app->m_overrideManager->ReloadConfig();
        app = (App*)WindowApp::m_instance;
    }
    if (app->m_weaponsManager != NULL) {
        app->m_weaponsManager->ReloadConfig();
        app = (App*)WindowApp::m_instance;
    }
    if (app->m_waveManager != NULL) {
        app->m_waveManager->ReloadRecommendations();
    }
}

void App::Shutdown()
{
    CGameAnalytics::logAppStop();
    if (m_saveManager != NULL && m_field1c8) {
        m_saveManager->Save(true);
    }
    CGameAnalytics::logTotalCurrency(CBH_Player::GetInstance()->m_id,
                                     CBH_Player::GetInstance()->GetStats(PLAYER_STATS_SOFT_CURRENCY),
                                     CGameAnalytics::CURRENCY_SOFT);
    CGameAnalytics::logTotalCurrency(CBH_Player::GetInstance()->m_id,
                                     CBH_Player::GetInstance()->GetStats(PLAYER_STATS_HARD_CURRENCY),
                                     CGameAnalytics::CURRENCY_HARD);
    SG_Home::GetInstance()->DumpAllArchetypeCharacters();
    DeleteGameRunningFile();
    if (m_sgInstance != NULL) {
        m_sgInstance->Destroy();
        if (m_sgInstance != NULL) {
            delete m_sgInstance;
            m_sgInstance = NULL;
        }
    }
    ICInAppPurchase* purchase = ICInAppPurchase::GetInstance();
    if (purchase != NULL) {
        delete purchase;
    }
    if (m_offersManager != NULL) {
        delete m_offersManager;
        m_offersManager = NULL;
    }
    if (m_tipManager != NULL) {
        delete m_tipManager;
        m_tipManager = NULL;
    }
    if (m_progressSystem != NULL) {
        delete m_progressSystem;
        m_progressSystem = NULL;
    }
    if (m_weaponsManager != NULL) {
        delete m_weaponsManager;
        m_weaponsManager = NULL;
    }
    if (m_grenadePackManager != NULL) {
        delete m_grenadePackManager;
        m_grenadePackManager = NULL;
    }
    if (m_grenadeManager != NULL) {
        delete m_grenadeManager;
        m_grenadeManager = NULL;
    }
    if (m_powerUpPackManager != NULL) {
        delete m_powerUpPackManager;
        m_powerUpPackManager = NULL;
    }
    if (m_powerUpManager != NULL) {
        delete m_powerUpManager;
        m_powerUpManager = NULL;
    }
    if (m_socialGiftManager != NULL) {
        delete m_socialGiftManager;
        m_socialGiftManager = NULL;
    }
    if (m_newsFeedManager != NULL) {
        delete m_newsFeedManager;
        m_newsFeedManager = NULL;
    }
    if (m_scriptManager != NULL) {
        delete m_scriptManager;
        m_scriptManager = NULL;
    }
    if (m_commonGameConfig != NULL) {
        delete m_commonGameConfig;
        m_commonGameConfig = NULL;
    }
    if (m_fiveDaysBonus != NULL) {
        delete m_fiveDaysBonus;
        m_fiveDaysBonus = NULL;
    }
    if (m_achievementManager != NULL) {
        delete m_achievementManager;
        m_achievementManager = NULL;
    }
    if (m_helper != NULL) {
        delete m_helper;
        m_helper = NULL;
    }
    if (m_iapCurrencyManager != NULL) {
        delete m_iapCurrencyManager;
        m_iapCurrencyManager = NULL;
    }
    if (m_energyManager != NULL) {
        delete m_energyManager;
        m_energyManager = NULL;
    }
    if (m_nationalityTypeManager != NULL) {
        delete m_nationalityTypeManager;
        m_nationalityTypeManager = NULL;
    }
    if (m_enemyWeaponTypeManager != NULL) {
        delete m_enemyWeaponTypeManager;
        m_enemyWeaponTypeManager = NULL;
    }
    if (m_personTypeManager != NULL) {
        delete m_personTypeManager;
        m_personTypeManager = NULL;
    }
    if (m_huntingInfo != NULL) {
        delete m_huntingInfo;
        m_huntingInfo = NULL;
    }
    if (m_locationManager != NULL) {
        delete m_locationManager;
        m_locationManager = NULL;
    }
    if (m_missionManager != NULL) {
        delete m_missionManager;
        m_missionManager = NULL;
    }
    if (m_zombieMissionManager != NULL) {
        delete m_zombieMissionManager;
        m_zombieMissionManager = NULL;
    }
    if (m_waveManager != NULL) {
        delete m_waveManager;
        m_waveManager = NULL;
    }
    if (m_questManager != NULL) {
        delete m_questManager;
        m_questManager = NULL;
    }
    if (m_personsGroupManager != NULL) {
        delete m_personsGroupManager;
        m_personsGroupManager = NULL;
    }
    if (m_sceneSetupsManager != NULL) {
        delete m_sceneSetupsManager;
        m_sceneSetupsManager = NULL;
    }
    if (m_balanceManager != NULL) {
        delete m_balanceManager;
        m_balanceManager = NULL;
    }
    if (m_sceneObjectManager != NULL) {
        delete m_sceneObjectManager;
        m_sceneObjectManager = NULL;
    }
    if (m_highscoreManager != NULL) {
        delete m_highscoreManager;
        m_highscoreManager = NULL;
    }
    if (m_soundManager != NULL) {
        delete m_soundManager;
        m_soundManager = NULL;
    }
    if (m_resourceManager != NULL) {
        delete m_resourceManager;
        m_resourceManager = NULL;
    }
    if (m_updateManager != NULL) {
        delete m_updateManager;
        m_updateManager = NULL;
    }
    if (m_overrideManager != NULL) {
        delete m_overrideManager;
        m_overrideManager = NULL;
    }
    if (m_leaderboardsManager != NULL) {
        delete m_leaderboardsManager;
        m_leaderboardsManager = NULL;
    }
    m_mediaCache.setSize(0);
    if (m_resMap != NULL) {
        delete m_resMap;
        m_resMap = NULL;
    }
    if (m_settings != NULL) {
        delete m_settings;
        m_settings = NULL;
    }
    CBH_Player::GetInstance()->ReleaseResources();
    if (m_visualCharacterTypeManager != NULL) {
        delete m_visualCharacterTypeManager;
        m_visualCharacterTypeManager = NULL;
    }
    if (m_surfaceManager != NULL) {
        m_surfaceManager->Clear();
        if (m_surfaceManager != NULL) {
            delete m_surfaceManager;
            m_surfaceManager = NULL;
        }
    }
    lua_close(m_lua);
    m_lua = NULL;
    if (m_privateData != NULL) {  // changed by fx2
        free(m_privateData);  // changed by fx2
        m_privateData = NULL;  // changed by fx2
    }
    if (m_buffer1d4 != NULL) {
        free(m_buffer1d4);
        m_buffer1d4 = NULL;
    }
    if (m_buffer1cc != NULL) {
        free(m_buffer1cc);
        m_buffer1cc = NULL;
    }
    if (m_buffer2f8 != NULL) {
        free(m_buffer2f8);
        m_buffer2f8 = NULL;
    }
    if (m_buffer1d8 != NULL) {
        free(m_buffer1d8);
        m_buffer1d8 = NULL;
    }
    if (m_buffer1d0 != NULL) {
        free(m_buffer1d0);
        m_buffer1d0 = NULL;
    }
    if (m_buffer2fc != NULL) {
        free(m_buffer2fc);
        m_buffer2fc = NULL;
    }
    if (m_buffer1e0 != NULL) {
        free(m_buffer1e0);
        m_buffer1e0 = NULL;
    }
    if (m_buffer1dc != NULL) {
        free(m_buffer1dc);
        m_buffer1dc = NULL;
    }
    if (m_notificationHandler != NULL) {
        delete m_notificationHandler;
        m_notificationHandler = NULL;
    }
    CNetMessageQueue_gServe::GetInstance()->atExit();
    m_world = NULL;
    if (m_saveManager != NULL) {
        delete m_saveManager;
        m_saveManager = NULL;
    }
    WindowApp::Shutdown();
}

void App::DeleteGameRunningFile()
{
    CStrWChar path;
    CFileUtil::GetApplicationDataPathForFile(path, gameRunningFilename);
    ICFileMgr::GetInstance()->Delete(path.m_string);
}

void App::OnPause()
{
    if (m_soundManager != NULL) {
        m_soundManager->Pause();
    }
}

void App::OnResume()
{
    if (m_soundManager != NULL) {
        m_soundManager->Resume();
    }
}

void App::OnEnterBackground()
{
    if (!m_initialized) {
        return;
    }
    CBH_Player* player = CBH_Player::m_staticInstance;
    if (player != NULL) {
        player->AddGameTime(GetActiveTime());
    }
    CGameAnalytics::logAppBackground();
    CGameAnalytics::logTotalCurrency(CBH_Player::GetInstance()->m_id,
                                     CBH_Player::GetInstance()->GetStats(PLAYER_STATS_SOFT_CURRENCY),
                                     CGameAnalytics::CURRENCY_SOFT);
    CGameAnalytics::logTotalCurrency(CBH_Player::GetInstance()->m_id,
                                     CBH_Player::GetInstance()->GetStats(PLAYER_STATS_HARD_CURRENCY),
                                     CGameAnalytics::CURRENCY_HARD);
    CTjManager::GetInstance()->OnAppEnterBackground();
    m_saveManager->Save(false);
    ((App*)WindowApp::m_instance)->m_soundManager->StopSounds();
    ((App*)WindowApp::m_instance)->m_soundManager->Pause(BACKGROUND_SOUNDS);
    CNGS::GetInstance()->Pause();
    CNetMessageQueue_gServe::GetInstance()->atExit();
}

void App::CreateGameRunningFile()
{
    CStrWChar path;
    CFileUtil::GetApplicationDataPathForFile(path, gameRunningFilename);
    ICFileMgr* files = ICFileMgr::GetInstance();
    ICFile* file = files->Open(path.m_string, ICFileMgr::AccessWrite);
    if (file != NULL) {
        files->Close(file);
    }
}

Window* App::NewDisplayWindow()
{
    return new DisplayWindow();
}

void App::Start()
{
    Window* mainLayer = new WindowTransparent(false);
    m_mainLayer = mainLayer;
    mainLayer->SetAlign(Window::ALIGN_NONE);
    m_desktop->AddToFront(m_mainLayer);
    Window* topLayer = new WindowTransparent(false);
    m_topLayer = topLayer;
    topLayer->ClearFlags(Window::FLAG_FOCUSABLE);
    m_topLayer->SetAlign(Window::ALIGN_NONE);
    m_desktop->AddToFront(m_topLayer);
    m_mainLayer->AddModal(new StartDelayWindow());
    m_field1c8 = true;
}

void App::UpdateTime()
{
    int frameTime = m_frameTimes[7];
    if (frameTime < 1) {
        frameTime = 1;
    }
    float time = (float)frameTime;
    if (frameTime > MAX_FRAME_TIME) {
        time = (float)MAX_FRAME_TIME;
    }
    m_updateFrameTime = frameTime;
    m_updateFrameTime = MathLib::Round(time * m_timeScale);
    m_updateTime = m_updateTime + m_updateFrameTime;
}

void App::Update()
{
    UpdateTickRate();
    ((App*)WindowApp::m_instance)->m_saveManager->Update();
    if (!m_pushStatisticsSent) {
        m_pushStatisticsDelay = m_pushStatisticsDelay - WindowApp::m_instance->m_frameTimeSeconds;
        if (m_pushStatisticsDelay <= 0.0f) {
            m_pushStatisticsDelay = PUSH_STATISTICS_DELAY;
            CStrWChar token;
    CGameApp::GetInstance()->m_pushTokenSource->GetIdentity(token);
            if (token.GetLength() != 0) {
                m_pushStatisticsSent = true;
                XString text(token.GetWCharPtr());
                XString::AnsiString ansiText(text.c_str());
                CGameAnalytics::SendPushStatistics(ansiText);
                CGameAnalytics::SendDeferredPushToken();
            }
        }
    }
    CNGS::GetInstance()->HandleUpdate(WindowApp::m_instance->m_updateFrameTime);
    m_gserveHandler->HandleUpdate();
    CNetMessageQueue_gServe::GetInstance()->HandleUpdate(WindowApp::m_instance->m_updateFrameTime);
    unsigned long elapsed = WindowApp::m_instance->m_updateFrameTime;
    TimeManager* timeManager = &m_timeManager;
    timeManager->update(elapsed);
    if (m_soundManager != NULL) {
        m_soundManager->Update(elapsed);
    }
    if (m_updateManager != NULL) {
        m_updateManager->update(elapsed);
    }
}

void App::Paint(ICGraphics2d& graphics)
{
}

void App::HandleRender()
{
    m_field37c = 0;
    HandlePaint();
}

void App::RenderWorld(const swerve::Graphics3D<CSwerveAssertingErrorHandler>& graphics,
                      const swerve::World<CSwerveAssertingErrorHandler>& world)
{
    if (!graphics.hasTarget()) {  // changed by fx2 (Graphics3D is a CSwerveGraphics3D handle)
        return;
    }
    graphics.renderer()->Render(world.m_interface);  // changed by fx2
}

void App::OnCommand(Event& event)
{
    int id = event.m_id;
    if (id == COMMAND_ACCELEROMETER) {
        // the command parameters carry the raw float bits of the acceleration
        float x = *(float*)&event.m_param1;
        float y = *(float*)&event.m_param2;
        float z = *(float*)&event.m_param3;
        // Ordered comparisons reset pending orientation for unordered acceleration.
        if (!(z > FACE_DOWN_Z)) {
            m_pendingOrientation = -1;
        } else {
            float absoluteX = MathLib::Fabs(x);
            float absoluteY = MathLib::Fabs(y);
            if (!(MathLib::Fabs(absoluteX - absoluteY) > ORIENTATION_MIN_DIFFERENCE)) {
                m_pendingOrientation = -1;
            } else {
                int orientation;
                if (x > y) {
                    orientation = 1;
                    if (absoluteY < absoluteX) {
                        orientation = 2;
                    }
                } else {
                    orientation = 3;
                    if (absoluteY < absoluteX) {
                        orientation = 4;
                    }
                }
                if (m_pendingOrientation == orientation) {
                    if (orientation != CApplet::m_pCore->GetDeviceOrientation()
                        && WindowApp::m_instance->m_timeSeconds - m_orientationChangeTime > ORIENTATION_DELAY) {
                        CApplet::m_pCore->SetDeviceOrientation(
                            (framework::ICCore::eDeviceOrientation)m_pendingOrientation);
                    }
                } else if (orientation == 2 || orientation == 4) {
                    m_pendingOrientation = orientation;
                    m_orientationChangeTime = WindowApp::m_instance->m_timeSeconds;
                }
            }
        }
        // acceleration in screen coordinates
        switch (CApplet::m_pCore->GetDeviceOrientation()) {
        case 1:
            m_acceleration.m_x = x;
            m_acceleration.m_y = y;
            break;
        case 2:
            m_acceleration.m_x = y;
            m_acceleration.m_y = -x;
            break;
        case 3:
            m_acceleration.m_x = -x;
            m_acceleration.m_y = -y;
            break;
        case 4:
            m_acceleration.m_x = -y;
            m_acceleration.m_y = x;
            break;
        default:
            event.Clear();
            return;
        }
        m_acceleration.m_z = z;
    } else if (id == COMMAND_TAPJOY) {
        if (TapjoyInterface::canDisplayInterface()) {
            TapjoyInterface::openTapjoyInterface();
        }
        return;
    } else if (id == COMMAND_SELECT_LANGUAGE) {
        ((App*)WindowApp::m_instance)->m_mainLayer->AddModal(new SelectLanguageDialog());
    } else {
        return;
    }
    event.Clear();
}

void App::Inc2DLayer()
{
}

void App::OnBeginWindowPaint(ICGraphics2d& graphics, Window* window)
{
}

void App::OnEndWindowPaint(ICGraphics2d& graphics, Window* window)
{
}

void App::OnBeginWindowPaint3D(ICGraphics2d& graphics, Window* window)
{
    const math::CMatrix2d* transform;
    const math::CVector2d* translation;
    graphics.GetTransform(&transform, &translation);
    m_savedTransform = *transform;
    m_savedTranslation = *translation;
    unsigned char scissorEnabled;
    ICGraphics::GetInstance()->GetRasterizerState()->GetScissorState(scissorEnabled, m_viewport.m_x, m_viewport.m_y,
                                                                    m_viewport.m_width, m_viewport.m_height);
    m_node->RenderEnd();
}

void App::OnEndWindowPaint3D(ICGraphics2d& graphics, Window* window)
{
    m_node->RenderBegin();
    ICGraphics::GetInstance()->GetRasterizerState()->SetScissor(m_viewport.m_x, m_viewport.m_y, m_viewport.m_width,
                                                                m_viewport.m_height);
    graphics.LoadTransform(&m_savedTransform, &m_savedTranslation);
}

bool App::IsEnoughFreeSpace()
{
    return ICFileMgr::GetInstance()->GetFreeDiskSpace() > MIN_FREE_SPACE;
}

CShopItem* App::GetItemByStringUId(const XString& id)
{
    CShopItem* item = ((App*)WindowApp::m_instance)->m_weaponsManager->GetItemByStringUId(id);
    if (item == NULL) {
        item = ((App*)WindowApp::m_instance)->m_grenadePackManager->GetItemByStringUId(id);
    }
    if (item == NULL) {
        item = ((App*)WindowApp::m_instance)->m_powerUpPackManager->GetItemByStringUId(id);
    }
    if (item == NULL) {
        item = ((App*)WindowApp::m_instance)->m_grenadeManager->GetItemByStringUId(id);
    }
    if (item == NULL) {
        // CShopItem is the historical opaque name here; the recovered managers use CItem.
        item = (CShopItem*)((App*)WindowApp::m_instance)->m_powerUpManager->GetItemByStringUId(id);
    }
    if (item == NULL) {
        item = (CShopItem*)((App*)WindowApp::m_instance)->m_iapCurrencyManager->GetItemByStringUId(id);
    }
    return item;
}

bool App::IsHD()
{
    unsigned short height = CApplet::m_UIScreenMainScreenHeight;
    unsigned short width = CApplet::m_UIScreenMainScreenWidth;
    unsigned short larger = height;
    if (width > height) {
        larger = width;
    }
    unsigned short smaller = height;
    switch (larger) {
    case 960:
        if (width < height) {
            smaller = width;
        }
        return smaller == 640;
    case 1024:
        if (width < height) {
            smaller = width;
        }
        return smaller == 768;
    case 800:
        if (width < height) {
            smaller = width;
        }
        return smaller == 480;
    }
    return false;
}

bool App::IsVGA()
{
    unsigned short height = CApplet::m_UIScreenMainScreenHeight;
    unsigned short width = CApplet::m_UIScreenMainScreenWidth;
    unsigned short larger = height;
    if (width > height) {
        larger = width;
    }
    if (larger != 960) {
        return false;
    }
    unsigned short smaller = height;
    if (width < height) {
        smaller = width;
    }
    return smaller == 640;
}

bool App::IsXGA()
{
    unsigned short height = CApplet::m_UIScreenMainScreenHeight;
    unsigned short width = CApplet::m_UIScreenMainScreenWidth;
    unsigned short larger = height;
    if (width > height) {
        larger = width;
    }
    if (larger != 1024) {
        return false;
    }
    unsigned short smaller = height;
    if (width < height) {
        smaller = width;
    }
    return smaller == 768;
}

bool App::IsWVGA()
{
    unsigned short height = CApplet::m_UIScreenMainScreenHeight;
    unsigned short width = CApplet::m_UIScreenMainScreenWidth;
    unsigned short larger = height;
    if (width > height) {
        larger = width;
    }
    if (larger != 800) {
        return false;
    }
    unsigned short smaller = height;
    if (width < height) {
        smaller = width;
    }
    return smaller == 480;
}

bool App::IsAtrix()
{
    return false;
}

bool App::IsKindle()
{
    return false;
}

int App::GetActiveTime() const
{
    return CStdUtil_Cocoa::GetTimeSeconds() - m_activeStartTime;
}

int App::ConsumeActiveTime()
{
    int now = CStdUtil_Cocoa::GetTimeSeconds();
    int start = m_activeStartTime;
    m_activeStartTime = now;
    return now - start;
}

void App::OnEnterForeground()
{
    if (!m_initialized) {
        return;
    }
    m_activeStartTime = CStdUtil_Cocoa::GetTimeSeconds();
    CGameAnalytics::logAppForeground();
    LocalPushManager::CancelLocalPushNotifications(false);
    CNGS::GetInstance()->Resume();
    CNetMessageQueue_gServe::GetInstance()->onStart();
    if (m_initialized) {
        ((App*)WindowApp::m_instance)->m_soundManager->Resume(BACKGROUND_SOUNDS);
        ((App*)WindowApp::m_instance)->m_iapCurrencyManager->UpdateIAPInfo(false);
        CBH_Player::GetInstance()->m_gameStats.NotifyAppForeground();
        ((App*)WindowApp::m_instance)->m_updateManager->checkUpdates();
    }
}

void App::LoadSGArchetype(int archetype, int character)
{
    unsigned char more = 1;
    SG_Home::GetInstance()->QueueArchetypeCharacter(archetype, character);
    while (more != 0) {
        if (SG_Home::GetInstance()->LoadQueued(2000, more) == 0) {
            break;
        }
    }
}

void App::StartGServerLight()
{
    CNGSLoginFlow* flow = CNGSLoginFlow::GetInstance();
    flow->Reset();
    VersionCheckStep* step = new VersionCheckStep();
    step->m_classId = 0x81a8233f;
    step->m_snType = -1;
    flow->Queue(step);
    flow->Start();
}
