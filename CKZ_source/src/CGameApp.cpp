// CGameApp.cpp - framework integration, application resources, purchases and notifications.
#include "CGameApp.h"
#include "JSON.h"
#include "CAppFactory.h"
#include "CResourceManager_v2.h"
#include "CAppExecutor.h"
#include "CExecutableRegistry.h"
#include "CInputMap.h"
#include "CRegistryItr.h"
#include "CRegistryAccelerateHandleQuery.h"
#include "CMessage.h"
#include "CEventListener.h"
#include "CApplet.h"
#include "App.h"
#include "AppNode.h"
#include "AppSettings.h"
#include "CProfileManager.h"
#include "ICAdManager.h"
#include "ICMediaPlayer.h"
#include "ICGraphics.h"
#include "ICGraphics2d.h"
#include "ICRasterizerState.h"
#include "ICRenderSurface.h"
#include "ICCore.h"
#include "TapjoyInterface.h"
#include "AdBannerSizes.h"
#include "CAppProperties.h"
#include "CFileUtil.h"
#include "ICFileMgr.h"
#include "ICLicenseMgr.h"
#include "ICMoviePlayer.h"
#include "CRandGen.h"
#include "CCrc32.h"
#include "CDemoMgr.h"
#include "CFontMgr.h"
#include "CPHInterface.h"
#include "CSwerve.h"
#include "CBH_Player.h"
#include "CGameAnalytics.h"
#include "CIAPCurrencyManager.h"
#include "CIAPCurrency.h"
#include "COffersManager.h"
#include "CDH_WeaponsManager.h"
#include "CGrenadeManager.h"
#include "CGrenade.h"
#include "CPowerUpManager.h"
#include "CPowerUp.h"
#include "CZombieDialogWindow.h"
#include "CDialogQueueWindow.h"
#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>

using namespace com::glu::platform;
using namespace com::glu::platform::framework;
using namespace com::glu::platform::components;
using namespace com::glu::platform::systems;
using namespace com::glu::platform::graphics;

bool CGameApp::syncServerFlag;
bool CGameApp::gserveDataConflict;

extern const wchar_t GAME_RES_FILE[];
static const unsigned int CGameApp_InterestingEvents[] = {
    0x67542658, 0x36754280, 0x8408f369, 0x6740942f, 0x54246de3, 0x75424363,
    0x518e1a3b, 0x5152a73b, 0x695fb416, 0x1e96b3a4, 0x0096b3eb, 0xc7424be6,
    0x43a39818, 0x8251b8de, 0x8918794c, 0x52a91971, 0x7b8428cd, 0x35c43e56,
    0x35cb8f1c, 0x35c38d1c, 0x5707b968, 0xad174cef, 0x24314258, 0x97200e9a,
    0xfd4836a5, 0x3f972120, 0x972028c1, 0x9720a3cb, 0x7214ff0d, 0x2074d009,
    0x97204784, 0x9720b34a, 0x5f61a84a, 0xd45f52de, 0x97f37ccf, 0xa8966e06,
    0x20174fb7, 0x172ebf3e, 0x172e3434, 0x71fd83f5, 0x172e9965, 0x172ec5fa,
    0x71ff1054, 0x0173eaf6, 0xd0467c9b, 0x016703b6, 0x490287b4, 0x9036abf0,
    0xbd152e06, 0x152f0ca3, 0xbd152f06, 0x152e0ca3, 0xfe50251e, 0xfe50251d,
    0xfe50251c, 0xfe50251b, 0x9f523292, 0x385a651b, 0
};
static const char PLAYHAVEN_APP_KEY[] = "daddc3f644b641fba5748013ec66480e";
static const char PLAYHAVEN_SECRET[] = "0176ff0d779844a9bc0d18b5ec5fc43f";
static const char PUSH_NOTIFICATIONS_APP_KEY[] = "inorL-_WQ1CHzDmscKRBqA";
static const char PUSH_NOTIFICATIONS_APP_SECRET[] = "qi9DknoeRBqoqTgiQ_3pWA";
static const char TAPJOY_APP_ID[] = "e363778a-687a-416c-9601-2f9af2323db0";
static const char TAPJOY_SECRET_KEY[] = "";
static const wchar_t PROPERTIES_FILE[] = L"properties.dat";

CApp* CApp::CreateInstance()
{
    return new CGameApp;
}

ICResourceManager* CAppFactory::CreateResourceManager()
{
    return new CResourceManager_v2;
}

void CAppExecutor::OnExecute()
{
    if (m_modeQualifier != 0) {
        return;
    }
    if (m_mode != 1) {
        if (m_mode != 0) {
            return;
        }
        m_registry->Run();
        CGameApp::GetInstance()->HandleUpdate();
        m_mode = 1;
    }
    if (glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES) == GL_FRAMEBUFFER_COMPLETE_OES) {
        CGameApp::GetInstance()->HandleRender();
        m_mode = 0;
    }
    else {
        ICGraphics::GetInstance()->SwapBuffers();
    }
}

CGameApp* CGameApp::GetInstance()
{
    return static_cast<CGameApp*>(CApplet::m_pApp);
}

CGameApp::CGameApp()
    : m_renderEnabled(1), m_displayProgram(NULL),
      m_displayMode(ICDisplayProgram::Mode::SortAllBackToFrontIgnoreTargetCommands),
      m_pushTokenSource(NULL)
{
    m_eventListener = NULL;
    m_rootNode = NULL;
}

CGameApp::~CGameApp()
{
}

void CGameApp::HandleUpdate()
{
    if (syncServerFlag) {
        CProfileManager* profile = ((App*)WindowApp::m_instance)->m_profileManager;
        if (profile->isReady() && profile->m_writeRequestsOutstanding == 0) {
            syncServerFlag = false;
        }
    }
    int elapsed = m_executor->m_frameMillis;
    ICAdManager* ads = ICAdManager::GetInstance();
    ads->HandleUpdate(elapsed);
    if (ads->IsFullScreen()) {
        return;
    }
    m_rootNode->HandleUpdate(elapsed);
    ICMediaPlayer::GetInstance()->HandleUpdate(elapsed);
    if (((App*)WindowApp::m_instance)->m_iapCurrencyManager != NULL) {
        for (int index = 0; index < m_pendingPurchases.size(); index++) {
            OnIAPComplete(m_pendingPurchases[index]);
        }
        m_pendingPurchases.removeAllElements();
    }
    ICInAppPurchase* purchases = ICInAppPurchase::GetInstance();
    if (purchases->IsTransactionActive()) {
        purchases->HandleUpdate(elapsed);
    }
    if (m_pendingNotifications.size() != 0 && App::IsInitialized() == 1) {
        HandlePushNotification(m_pendingNotifications[0].m_data);
        m_pendingNotifications.removeElementAt(0);
    }
}

void CGameApp::HandleRender()
{
    if (!m_renderEnabled) {
        return;
    }
    ICGraphics* graphics = ICGraphics::GetInstance();
    ICGraphics2d::GetInstance();
    ICRasterizerState* state = graphics->GetRasterizerState();
    ICRenderSurface* target = graphics->GetTargetSurface();
    unsigned int width;
    unsigned int height;
    target->GetWidthAndHeight(width, height);
    graphics->SetViewport(0, 0, (short)width, (short)height);
    graphics->SetClearScissor(0, 0, (short)width, (short)height);
    graphics->SetClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    graphics->ClearBuffers(0x70000);
    ICAdManager* ads = ICAdManager::GetInstance();
    if (ads->IsFullScreen()) {
        state->EnableScissorTest(1);
        state->EnableBlending(1);
        state->SetBlendFactors(ICRasterizerState::BlendFactor_SrcAlpha, ICRasterizerState::BlendFactor_OneMinusSrcAlpha);
        state->SetDepthTestFunc(ICRasterizerState::DepthTestFunc_None);
        ads->HandleRender();
        return;
    }
    m_displayProgram = graphics->Begin(m_displayMode, 0, 1);
    state->EnableScissorTest(1);
    state->EnableBlending(1);
    state->SetBlendFactors(ICRasterizerState::BlendFactor_SrcAlpha, ICRasterizerState::BlendFactor_OneMinusSrcAlpha);
    state->SetDepthTestFunc(ICRasterizerState::DepthTestFunc_None);
    m_rootNode->HandleRender();
    graphics->End(*m_displayProgram);
    m_displayProgram = NULL;
    if (ads->IsVisible()) {
        ads->HandleRender();
    }
    graphics->SwapBuffers();
    ICRenderSurface::Description description;
    target->GetDescription(description);
    if (description.m_colorKind == 3 || description.m_colorKind == 4) {
        m_rootNode->HandleEvent(0x24314258, 0, 0);
    }
}

unsigned char AccelerateEventSpecificListenerQuery(unsigned int, int& first, int& second)
{
    // Original 0xddaf8 stores zero in both outputs and returns zero.
    first = 0;
    second = 0;
    return 0;
}

int CAppExecutor::InitRegistry()
{
    CExecutableRegistry* registry = m_registry;
    registry->Add(*registry->CreateSystemElement(0x957ce610, new CExecutableRegistry, CExecutableRegistry::ClassId, 0x3fffffff));
    registry->Add(*registry->CreateSystemElement(0x92484fb2, new CInputMap, CInputMap::ClassId, 0x3fffffff));
    registry->Add(*registry->CreateSystemElement(0xc84e9c27, new CInputMap, CInputMap::ClassId, 0x3fffffff));
    return 0;
}

void CAppExecutor::ReleaseRegistry()
{
    CRegistry* registry = m_registry;
    CSystemElement* element = NULL;
    CRegistryItr iterator(*registry);
    while (!iterator.IsDone()) {
        CSystemElement* current = &*iterator;
        core::CClass* object = static_cast<core::CClass*>(current->GetObject());
        if (object->m_classId == CEvent::ClassId || object->m_classId == CMessage::ClassId) {
            delete object;
            CSystemElement* next = current->m_next;
            registry->Remove(*current);
            if (next == NULL) {
                next = current;
            }
            delete current;
            iterator.m_current = next;
        }
        else {
            iterator++;
        }
    }
    if (registry->Query(0, 0xc84e9c27, &element)) {
        registry->Remove(*element);
        delete static_cast<core::CClass*>(element->GetObject());
        delete element;
    }
    if (registry->Query(0, 0x92484fb2, &element)) {
        registry->Remove(*element);
        delete static_cast<core::CClass*>(element->GetObject());
        delete element;
    }
    if (registry->Query(0, 0x957ce610, &element)) {
        registry->Remove(*element);
        delete static_cast<core::CClass*>(element->GetObject());
        delete element;
    }
}

int CGameApp::OnInit()
{
    XString::Init();
    CStrWChar platform = CApplet::m_pCore->GetPlatformIdentifier();
    if (wcscmp(platform.GetWCharPtr(), L"iPhone1,2") == 0 ||
        wcscmp(platform.GetWCharPtr(), L"iPod1,2") == 0 ||
        wcscmp(platform.GetWCharPtr(), L"iPod2,1") == 0) {
        App::m_platformType = 0;
    }
    m_singletons = new CHash;
    m_singletons->Init(20, 20, 1);
    CreatePools();
    m_subsystem14 = new CRegistry;
    if (m_subsystem14 != NULL) {
        InitRegistry();
    }
    ICResourceManager* resources = ICResourceManager::GetInstance();
    m_subsystem18 = reinterpret_cast<core::CClass*>(resources);
    if (resources == NULL) {
        OnFree();
        return 1;
    }
    bool success = resources->Initialize(ICResourceManager::DefaultTableSize,
                                         ICResourceManager::DefaultNumOfElementsInInitialAlloc, 1) != 0;
    CResource* resource;
    bool languageLoaded = false;
    if (CApplet::m_pCore->CanDetectLanguage()) {
        CStrChar language;
        CApplet::m_pCore->DetectLanguage(language);
        languageLoaded = resources->CreateResource((language + ".big").GetCharPtr(), &resource) != 0;
    }
    if (!languageLoaded && !resources->CreateResource("en.big", &resource)) {
        success = false;
    }
    if (App::IsXGA() || App::IsVGA() == 1) {
        if (!resources->CreateResource("vga.big", &resource)) {
            success = false;
        }
    }
    else if (App::IsWVGA() == 1) {
        if (!resources->CreateResource("wvga.big", &resource)) {
            success = false;
        }
    }
    unsigned int archive = resources->CreateResource(GAME_RES_FILE, &resource);
    if (!success || archive == 0) {
        return 1;
    }
    m_executor = new CAppExecutor;
    if (m_executor != NULL) {
        m_executor->Initialize();
    }
    m_eventListener = new CEventListener;
    m_eventListener->m_object = this;
    m_eventListener->m_handle = core::CStringToKey("GameApp", 0) | 0x80000000;
    m_eventListener->m_callback = EventCB;
    m_eventListener->Register(CGameApp_InterestingEvents, 0x3fffffff);
    ICLicenseMgr::GetInstance();
    ICFileMgr::GetInstance();
    ICMediaPlayer::GetInstance()->Initialize(1, 16, 44100, 7000, 10, 0);
    ICMoviePlayer::GetInstance();
    core::CRandGen::GetInstance();
    CCrc32::GetInstance();
    CAppProperties* properties = NULL;
    CApplet::m_pApp->m_singletons->Find(CAppProperties::SingletonId, (void**)&properties);
    if (properties == NULL) {
        properties = new CAppProperties;
    }
    CDemoMgr::GetInstance();
    CFontMgr::GetInstance();
    if (CApplet::m_pCore->AutoRotationIsActive()) {
        CApplet::m_pCore->AutoRotationOrientationEnable(ICCore::AutoRotation_LandscapeRight);
        CApplet::m_pCore->AutoRotationOrientationEnable(ICCore::AutoRotation_LandscapeLeft);
    }
    else {
        CApplet::m_pCore->SetDeviceOrientation(ICCore::DeviceOrientation_LandscapeLeft);
    }
    ICGraphics* graphics = ICGraphics::GetInstance();
    ICGraphics::InitParam parameters[] = {{0, 0}};
    graphics->Initialize(parameters);
    graphics->ClearBuffers(0x70000);
    graphics->SwapBuffers();
    graphics->ClearBuffers(0x70000);
    ICGraphics2d::GetInstance()->Initialize();
    ICAdManager::GetInstance()->Init(15052, 15053);
    CPHInterface::init(PLAYHAVEN_APP_KEY, PLAYHAVEN_SECRET);
    m_pushTokenSource = ICNotification::CreateInstance();
    if (!m_pushTokenSource->IsEnabled()) {
        m_pushTokenSource->Register(ICNotification::Type_Push, CStrWChar(PUSH_NOTIFICATIONS_APP_KEY).GetWCharPtr(),
            CStrWChar(PUSH_NOTIFICATIONS_APP_SECRET).GetWCharPtr(), NULL, NULL);
    }
    // This list and the following platform string are constructed in the original but not queried.
    Vector<XString> deviceModels;
    deviceModels.addElement(XString(L"iPhone1,1"));
    deviceModels.addElement(XString(L"iPhone1,2"));
    deviceModels.addElement(XString(L"iPhone2,1"));
    deviceModels.addElement(XString(L"iPod1,1"));
    deviceModels.addElement(XString(L"iPod2,1"));
    deviceModels.addElement(XString(L"iPod3,1"));
    deviceModels.addElement(XString(L"iPod4,1"));
    deviceModels.addElement(XString(L"iPad1,1"));
    XString model(CApplet::m_pCore->GetPlatformIdentifier().GetWCharPtr());
    TapjoyInterface::initialize(TAPJOY_APP_ID, TAPJOY_APP_ID, TAPJOY_SECRET_KEY, true);
    TapjoyInterface::registerVideoCallback(OnTapjoyVideo, NULL);
    m_rootNode = new AppNode;
    return 0;
}

void CGameApp::OnFree()
{
    TapjoyInterface::destroy();
    delete m_rootNode;
    m_rootNode = NULL;
    delete m_pushTokenSource;
    m_pushTokenSource = NULL;
    delete CFontMgr::GetInstance();
    ICAdManager* ads = ICAdManager::GetInstance();
    if (ads != NULL) {
        ads->Destroy();
        delete ads;
    }
    CPHInterface::destroy();
    delete CSwerve::GetInstance();
    ICGraphics2d* graphics2d = ICGraphics2d::GetInstance();
    if (graphics2d != NULL) {
        graphics2d->Destroy();
        delete graphics2d;
    }
    ICGraphics* graphics = ICGraphics::GetInstance();
    if (graphics != NULL) {
        graphics->Destroy();
        delete graphics;
    }
    delete ICMediaPlayer::GetInstance();
    delete ICMoviePlayer::GetInstance();
    ICResourceManager* resources = reinterpret_cast<ICResourceManager*>(m_subsystem18);
    if (resources != NULL) {
        resources->ReleaseResource(GAME_RES_FILE);
        resources->Destroy();
        delete resources;
        m_subsystem18 = NULL;
    }
    if (m_eventListener != NULL) {
        m_eventListener->UnregisterAll();
        delete m_eventListener;
        m_eventListener = NULL;
    }
    delete m_executor;
    m_executor = NULL;
    if (m_subsystem14 != NULL) {
        ReleaseRegistry();
        delete m_subsystem14;
        m_subsystem14 = NULL;
    }
    DestroyPools();
    if (m_singletons != NULL) {
        int bucketCount = m_singletons->m_bucketCount;
        for (int bucket = 0; bucket < bucketCount; bucket++) {
            CHash::Entry* entry = m_singletons->m_buckets[bucket];
            while (entry != NULL) {
                core::CClass* object = static_cast<core::CClass*>(entry->m_value);
                entry = entry->m_next;
                delete object;
            }
        }
        delete m_singletons;
        m_singletons = NULL;
    }
    XString::Shutdown();
}

unsigned char CGameApp::HandleEvent(unsigned int id, unsigned int parameter1, unsigned int parameter2)
{
    switch (id) {
    case 0x97f37ccf:
        ICGraphics::GetInstance()->End(*m_displayProgram);
        return 0;
    case 0xa8966e06:
        ICGraphics::GetInstance()->Sync();
        ICGraphics::GetInstance()->Begin(*m_displayProgram, m_displayMode, 0, 1);
        return 0;
    case 0xc7424be6:
    case 0x0096b3eb:
    case 0x1e96b3a4:
    case 0x43a39818:
    case 0x518e1a3b:
        return m_rootNode->HandleEvent(id, parameter1, parameter2);
    case 0x36754280:
        m_rootNode->Stop();
        ICMediaPlayer::GetInstance()->Stop(0);
        ICMediaPlayer::GetInstance()->StopVibrate(0);
        return 1;
    case 0x54246de3:
        ICMediaPlayer::GetInstance()->HandleSuspend();
        m_rootNode->HandleEvent(id, parameter1, parameter2);
        return 1;
    case 0x5f70b0e3:
        if (parameter1 != 0) {
            CStrWChar* notification = reinterpret_cast<CStrWChar*>(parameter1);
            XString text(notification->GetWCharPtr());
            XString::AnsiString unused(text.m_data);
            if (!App::IsInitialized()) {
                m_pendingNotifications.addElement(text);
            }
            else {
                HandlePushNotification(notification->GetWCharPtr());
            }
        }
        return 0;
    case 0x67542658: {
        CApplet::m_pApp->m_executor->SetTimerMillis(5);
        void* found = NULL;
        CApplet::m_pApp->m_singletons->Find(CAppProperties::SingletonId, &found);
        CAppProperties* properties = (CAppProperties*)found;
        if (properties == NULL) {
            properties = new CAppProperties;
        }
        CStrWChar path;
        CFileUtil::GetApplicationDataPathForFile(path, PROPERTIES_FILE);
        if (!ICFileMgr::GetInstance()->Exists(path.GetWCharPtr())) {
            CFileUtil::GetApplicationPathForFile(path, PROPERTIES_FILE);
        }
        if (ICFileMgr::GetInstance()->Exists(path.GetWCharPtr())) {
            CFileInputStream stream;
            if (stream.Open(path.GetWCharPtr())) {
                properties->m_properties.CollectOverriddenKeysInto(&properties->m_overriddenKeys);
                properties->m_properties.Load(stream, 0);
            }
        }
        CDemoMgr::GetInstance()->Read();
        m_rootNode->Start();
        return 1;
    }
    case 0x75424363:
        ICMediaPlayer::GetInstance()->HandleResume();
        m_rootNode->HandleEvent(id, parameter1, parameter2);
        return 1;
    case 0x5152a73b: {
        int handled = m_rootNode->HandleEvent(id, parameter1, parameter2);
        CApplet::m_pCore->ExitApp(ICCore::ExitStatus_Normal);
        return handled;
    }
    case 0x695fb416:
        return 0;
    }
    ICAdManager* ads = ICAdManager::GetInstance();
    if (ads->IsVisible() && ads->IsFullScreen()) {
        return 1;
    }
    return m_rootNode->HandleEvent(id, parameter1, parameter2);
}

unsigned char CGameApp::EventCB(CEvent& event, void* listener)
{
    CMessage& message = static_cast<CMessage&>(event);
    CGameApp* app = static_cast<CGameApp*>(static_cast<CEventListener*>(listener)->m_object);
    return app->HandleEvent(event.m_eventId, (unsigned int)message.m_arguments[0].m_data,
                            (unsigned int)message.m_arguments[1].m_data);
}

void CGameApp::CreatePools() const
{
    CEventPool::GetInstance()->Init(20, 0x2c, 1);
    CMessagePool::GetInstance()->Init(20, 0x15c, 1);
}

void CGameApp::DestroyPools() const
{
    CMessagePool* messages = CMessagePool::GetInstance();
    delete messages;
    CEventPool* events = CEventPool::GetInstance();
    delete events;
}

int CGameApp::InitRegistry()
{
    CSystem* root = static_cast<CSystem*>(CApplet::m_pApp->m_subsystem14);
    CRegistry* events = new CRegistry;
    events->SetAccelerator(new CRegistryAccelerateHandleQuery(150, 50));
    root->Add(*root->CreateSystemElement(0xf762ddf9, events, CRegistry::ClassId, 0x3fffffff));
    return 0;
}

void CGameApp::ReleaseRegistry()
{
    CSystem* root = static_cast<CSystem*>(CApplet::m_pApp->m_subsystem14);
    CSystemElement* element = NULL;
    if (root->Query(0, 0xf762ddf9, &element)) {
        root->Remove(*element);
        delete static_cast<core::CClass*>(element->GetObject());
        delete element;
    }
}

void CGameApp::OnTapjoyVideo(unsigned char state, void*)
{
    ((App*)WindowApp::m_instance)->m_settings->SetSoundEnabled(state == 0);
    ((App*)WindowApp::m_instance)->m_settings->SetMusicEnabled(state == 0);
}

void CGameApp::ReInit()
{
    // Original 0xdf0a4: bx lr, followed by alignment nop.
}

void CGameApp::OnTimer()
{
    if (m_executor != NULL) {
        m_executor->Run();
    }
}

void CGameApp::OnSystemEvent(unsigned int id, unsigned int parameter1, unsigned int parameter2)
{
    CMessage* message = new CMessage(this, id, 0, 2, CMessage::ArgumentType_UInt32, 0, parameter1,
                                    CMessage::ArgumentType_UInt32, 0, parameter2);
    message->Run();
    switch (id) {
    case 0x36754280: m_executor->OnStop(); break;
    case 0x67542658:
        ICGraphics::GetInstance()->ClearBuffers(0x70000);
        m_executor->OnStart();
        break;
    case 0x54246de3: m_executor->OnSuspend(); break;
    case 0x75424363:
        m_executor->OnResume();
        TapjoyInterface::onResume();
        break;
    case 0x5f70b0e3:
        if (parameter1 != 0) {
            CStrWChar* notification = reinterpret_cast<CStrWChar*>(parameter1);
            XString text(notification->GetWCharPtr());
            XString::AnsiString unused(text.m_data);
            if (!App::IsInitialized()) {
                m_pendingNotifications.addElement(text);
            }
            else {
                HandlePushNotification(notification->GetWCharPtr());
            }
        }
        break;
    }
}

/** Keep the gift count as a value parameter before the encrypted setter takes its address. */
static void SetPushGiftCount(Encrypted<int>& storedCount, int count)
{
    storedCount.Set(count);
}

void CGameApp::HandlePushNotification(const wchar_t* text)
{
    if (text == NULL) {
        return;
    }
    CGameAnalytics::logPushNotification(false);
    JSONValue* root = JSONValue::Parse(CString<wchar_t>(text));
    JSONObject* object = root->IsObject();
    if (object != NULL) {
        XString title(L"PUSH NOTIFICATION");
        XString message;
        int notificationId = 0;
        JSONValue* id = object->GetValue(L"_");
        if (id != NULL && id->IsString() != NULL) {
            XString identifier(id->IsString()->m_text.m_characters.m_data);
            notificationId = XString::HashData(identifier.m_data, identifier.ByteSize());
        }
        if (!CBH_Player::GetInstance()->IsPushNotificationHandled(notificationId)) {
            CBH_Player::GetInstance()->OnPushNotificationHandled(notificationId);
            JSONValue* aps = object->GetValue(L"aps");
            if (aps != NULL && aps->IsObject() != NULL) {
                JSONValue* alert = aps->IsObject()->GetValue(L"alert");
                if (alert != NULL && alert->IsString() != NULL) {
                    title = alert->IsString()->m_text.m_characters.m_data;
                }
            }
            JSONValue* gift = object->GetValue(L"gift");
            if (gift != NULL && gift->IsObject() != NULL) {
                for (int index = 0; index < gift->IsObject()->m_keys.Size(); index++) {
                    const wchar_t* key = gift->IsObject()->m_keys[index]->m_text.m_characters.m_data;
                    JSONValue* value = gift->IsObject()->m_values[index];
                    if (value->IsString() == NULL) {
                        continue;
                    }
                    if (wcscmp(key, L"xp gift") == 0) {
                        int amount = XString(value->IsString()->m_text.m_characters.m_data).ToInt();
                        message = message + XString::Format(Window::ResString("IDS_PUSH_GIFT_XP").m_data, amount);
                        message = message + L"\n";
                        CBH_Player::GetInstance()->AddStats((PLAYER_STATS_TYPE)3, amount, false, true);
                    }
                    else if (wcscmp(key, L"soft gift") == 0) {
                        int amount = XString(value->IsString()->m_text.m_characters.m_data).ToInt();
                        message = message + XString::Format(Window::ResString("IDS_PUSH_GIFT_SOFT").m_data, amount);
                        message = message + L"\n";
                        CBH_Player::GetInstance()->AddStats(PLAYER_STATS_SOFT_CURRENCY, amount, false, true);
                    }
                    else if (wcscmp(key, L"hard gift") == 0) {
                        int amount = XString(value->IsString()->m_text.m_characters.m_data).ToInt();
                        message = message + XString::Format(Window::ResString("IDS_PUSH_GIFT_HARD").m_data, amount);
                        message = message + L"\n";
                        CBH_Player::GetInstance()->AddStats(PLAYER_STATS_HARD_CURRENCY, amount, false, true);
                    }
                    else if (wcscmp(key, L"weapon gift") == 0) {
                        int weaponId = XString(value->IsString()->m_text.m_characters.m_data).ToInt();
                        CItem* weapon = ((App*)WindowApp::m_instance)->m_weaponsManager->getItemById(weaponId);
                        if (weapon != NULL) {
                            message = message + weapon->GetName();
                            message = message.Replace(L"&quot;", L"\"", true);
                            message = message + L"\n";
                            weapon->setAvailable(true);
                        }
                    }
                    else if (wcscmp(key, L"grenade gift") == 0) {
                        int grenadeId = XString(value->IsString()->m_text.m_characters.m_data).ToInt();
                        CGrenade* grenade = static_cast<CGrenade*>(((App*)WindowApp::m_instance)->m_grenadeManager->getItemById(grenadeId));
                        if (grenade != NULL) {
                            message = message + XString(L"3x ");
                            message = message + grenade->GetName();
                            message = message.Replace(L"&quot;", L"\"", true);
                            message = message + L"\n";
                            grenade->SetCount(grenade->GetCount() + 3);
                        }
                    }
                    else if (wcscmp(key, L"powerup gift") == 0) {
                        int powerUpId = XString(value->IsString()->m_text.m_characters.m_data).ToInt();
                        CPowerUp* powerUp = static_cast<CPowerUp*>(((App*)WindowApp::m_instance)->m_powerUpManager->getItemById(powerUpId));
                        if (powerUp != NULL) {
                            message = message + XString(L"3x ");
                            message = message + powerUp->GetName();
                            message = message.Replace(L"&quot;", L"\"", true);
                            message = message + L"\n";
                            SetPushGiftCount(powerUp->m_count, powerUp->GetCount() + 3);
                        }
                    }
                }
                if (message.Length() > 0) {
                    class MessageBox : public CZombieDialogWindow {
                    public:
                        MessageBox() : CZombieDialogWindow(3)
                        {
                            ((App*)WindowApp::m_instance)->TimePause();
                        }
                        virtual ~MessageBox()
                        {
                            ((App*)WindowApp::m_instance)->TimeResume();
                        }
                    };
                    MessageBox* dialog = new MessageBox;
                    dialog->SetTitle(title);
                    dialog->SetText(message, false, NULL);
                    dialog->AddButton(CZombieDialogWindow::BUTTON_OK, 0xf9394e27, 0, 0, true, 20);
                    Window* layer = ((App*)WindowApp::m_instance)->m_mainLayer;
                    if (layer != NULL) {
                        layer->AddToFront(dialog);
                    }
                    WindowApp::HandleTunnelCommand(0x3e59479d, 0, 0, 0);
                }
            }
        }
    }
    delete root;
}

void CGameApp::OnIAPComplete(const XString& productId)
{
    XString item = XString::Format(L"%02d_%s",
        ((App*)WindowApp::m_instance)->m_iapCurrencyManager->getIAPCurrencyByAppleId(productId)->getId(),
        ((App*)WindowApp::m_instance)->m_iapCurrencyManager->getIAPCurrencyByAppleId(productId)->GetName().m_data);
    ((App*)WindowApp::m_instance)->m_iapCurrencyManager->getIAPCurrencyByAppleId(productId)->OnIAPBought();
    WindowApp::HandleTunnelCommand(0xbe344666, 0, 0, 0);
}

void CGameApp::HandleIAPPurchase(int cash, int credits)
{
    XString title = Window::ResString("IDS_SHOP_IAP_SUCCESS_TITLE");
    XString cashName = Window::ResString("IDS_SHOP_IAP_CASH");
    XString creditsName = Window::ResString("IDS_SHOP_IAP_CREDITS");
    XString format = Window::ResString("IDS_SHOP_IAP_SUCCESS");
    // ?: kept on purpose: original constructs one result from two Format calls (tail-merged at 0xdfff04);
    // an if/else with assignment cannot produce that shape.
    XString text = (cash != 0)
        ? XString::Format(format.m_data, cash, cashName.m_data)
        : XString::Format(format.m_data, credits, creditsName.m_data);
    CZombieDialogWindow* dialog = new CZombieDialogWindow(3);
    dialog->SetTitle(title);
    dialog->SetText(text, false, NULL);
    dialog->AddButton(CZombieDialogWindow::BUTTON_OK, 0, 0, 0, true, 20);
    CDialogQueueWindow::AddDialogInQueue(dialog);
}

void CGameApp::SyncTransactionsComplete(SyncTransactionsState,
    const core::CVector<const ICInAppPurchaseTransaction*>&)
{
    // Original 0xe0078 (and its 0xe007c thunk): bx lr and alignment nop.
}

void CGameApp::RequestProductDataComplete(RequestProductDataState state,
    const core::CVector<const ICInAppPurchaseProduct*>& products)
{
    if (((App*)WindowApp::m_instance)->m_iapCurrencyManager == NULL) {
        return;
    }
    switch (state) {
    case REQUEST_PRODUCT_DATA_SUCCESS:
        ((App*)WindowApp::m_instance)->m_iapCurrencyManager->SetAllUnavailable();
        for (int index = 0; index < products.Size(); index++) {
            const ICInAppPurchaseProduct* product = products[index];
            CIAPCurrency* currency = ((App*)WindowApp::m_instance)->m_iapCurrencyManager->getIAPCurrencyByAppleId(
                XString(product->GetProductIdentifier().GetWCharPtr()));
            currency->setAvailable(true);
            currency->m_price = product->GetPrice();
            currency->m_currencySymbol = XString(product->GetCurrencySymbol().GetWCharPtr());
        }
        WindowApp::HandleTunnelCommand(0xff403571, 0, 0, 0);
        break;
    case REQUEST_PRODUCT_DATA_TIMEOUT:
    case REQUEST_PRODUCT_DATA_FAILED:
        ((App*)WindowApp::m_instance)->m_iapCurrencyManager->SetAllUnavailable();
        WindowApp::HandleTunnelCommand(0xc24364a4, 0, 0, 0);
        break;
    }
}

void CGameApp::BuyProductComplete(BuyProductState state, const ICInAppPurchaseTransaction* transaction)
{
    if (state == BUY_PRODUCT_SUCCESS) {
        XString productId(transaction->GetProductIdentifier().GetWCharPtr());
        CIAPCurrency* currency = ((App*)WindowApp::m_instance)->m_iapCurrencyManager->getIAPCurrencyByAppleId(productId);
        int bonus = 0;
        if (currency->m_fromOneTimeOffer) {
            bonus = ((App*)WindowApp::m_instance)->m_offersManager->m_bonus;
        }
        XString item = XString::Format(L"%02d_%s",
            ((App*)WindowApp::m_instance)->m_iapCurrencyManager->getIAPCurrencyByAppleId(productId)->getId(),
            ((App*)WindowApp::m_instance)->m_iapCurrencyManager->getIAPCurrencyByAppleId(productId)->GetName().m_data);
        if (currency->m_cashGain != 0) {
            HandleIAPPurchase(currency->m_cashGain + bonus, 0);
            CGameAnalytics::logIAPPurchaseSuccessful(item, currency->m_cashGain, (CGameAnalytics::ECurrencyType)0);
        }
        else if (currency->m_creditsGain != 0) {
            HandleIAPPurchase(0, currency->m_creditsGain + bonus);
            CGameAnalytics::logIAPPurchaseSuccessful(item, currency->m_creditsGain, (CGameAnalytics::ECurrencyType)1);
        }
        if (((App*)WindowApp::m_instance)->m_iapCurrencyManager != NULL) {
            OnIAPComplete(productId);
        }
        else {
            m_pendingPurchases.addElement(productId);
        }
        return;
    }
    if (state == BUY_PRODUCT_CANCELLED) {
        if (((App*)WindowApp::m_instance)->m_iapCurrencyManager != NULL) {
            XString productId(transaction->GetProductIdentifier().GetWCharPtr());
            CIAPCurrency* currency = ((App*)WindowApp::m_instance)->m_iapCurrencyManager->getIAPCurrencyByAppleId(productId);
            if (currency != NULL) {
                XString item = XString::Format(L"%02d_%s",
                    ((App*)WindowApp::m_instance)->m_iapCurrencyManager->getIAPCurrencyByAppleId(productId)->getId(),
                    ((App*)WindowApp::m_instance)->m_iapCurrencyManager->getIAPCurrencyByAppleId(productId)->GetName().m_data);
                if (currency->m_cashGain != 0) {
                    CGameAnalytics::logIAPPurchaseCanceled(item, currency->m_cashGain, (CGameAnalytics::ECurrencyType)0);
                }
                else if (currency->m_creditsGain != 0) {
                    CGameAnalytics::logIAPPurchaseCanceled(item, currency->m_creditsGain, (CGameAnalytics::ECurrencyType)1);
                }
            }
        }
        WindowApp::HandleTunnelCommand(0x5f9d3f54, 0, 0, 0);
        return;
    }
    if (state == BUY_PRODUCT_FAILED) {
        XString productId(transaction->GetProductIdentifier().GetWCharPtr());
        CIAPCurrency* currency = ((App*)WindowApp::m_instance)->m_iapCurrencyManager->getIAPCurrencyByAppleId(productId);
        if (currency != NULL) {
            XString item = XString::Format(L"%02d_%s", currency->getId(), currency->GetName().m_data);
            if (currency->m_cashGain != 0) {
                CGameAnalytics::logIAPPurchaseFailed(item, currency->m_cashGain, (CGameAnalytics::ECurrencyType)0);
            }
            else if (currency->m_creditsGain != 0) {
                CGameAnalytics::logIAPPurchaseFailed(item, currency->m_creditsGain, (CGameAnalytics::ECurrencyType)1);
            }
        }
        WindowApp::HandleTunnelCommand(0x659570dc, 0, 0, 0);
    }
}

void JSONValue::Cleanup(CString<wchar_t>& text)
{
    CString<wchar_t> cleaned;
    bool quoted = false;
    while (text.Length() != 0) {
        wchar_t character = text[0];
        if (character == L'"') {
            quoted = !quoted;
        }
        else if ((character == L' ' || (unsigned int)(character - 7) <= 6) && !quoted) {
            character = 0;
        }
        if (character != 0) {
            cleaned.Append(character);
        }
        text.RemoveRange(0, 1);
    }
    text = cleaned;
}

CString<wchar_t> JSONValue::ExtractFirstItem(CString<wchar_t>& text)
{
    CString<wchar_t> item;
    text.Trim(L' ');
    int depth = 0;
    bool quoted = false;
    while (text.Length() != 0) {
        wchar_t character = text[0];
        switch (character) {
        case L'"':
            quoted = !quoted;
            break;
        case L'[':
        case L'{':
            if (!quoted) {
                depth++;
            }
            break;
        case L']':
        case L'}':
            if (!quoted) {
                depth--;
            }
            break;
        case L',':
        case L';':
            if (depth == 0 && !quoted) {
                text.RemoveRange(0, 1);
                return item;
            }
            break;
        default:
            if ((character == L' ' || (unsigned int)(character - 7) <= 6) && !quoted) {
                character = 0;
            }
            break;
        }
        if (character != 0) {
            item.Append(character);
        }
        text.RemoveRange(0, 1);
    }
    return item;
}

CString<wchar_t> JSONValue::ExtractKey(CString<wchar_t>& text)
{
    CString<wchar_t> key;
    text.Trim(L' ');
    bool quoted = false;
    while (text.Length() != 0) {
        wchar_t character = text[0];
        if (character == L'"') {
            quoted = !quoted;
        }
        else if ((character == L':' || character == L'=') && !quoted) {
            text.RemoveRange(0, 1);
            return key;
        }
        if (character != 0) {
            key.Append(character);
        }
        text.RemoveRange(0, 1);
    }
    return key;
}

JSONValue* JSONValue::Parse(CString<wchar_t> text)
{
    Cleanup(text);
    if (text[0] == L'{') {
        while (text.Length() != 0 && text[text.Length() - 1] != L'}') {
            text.RemoveRange(text.Length() - 1, text.Length());
        }
        return new JSONObject(text);
    }
    if (text[0] == L'[') {
        while (text.Length() != 0 && text[text.Length() - 1] != L']') {
            text.RemoveRange(text.Length() - 1, text.Length());
        }
        return new JSONArray(text);
    }
    return new JSONString(text);
}

