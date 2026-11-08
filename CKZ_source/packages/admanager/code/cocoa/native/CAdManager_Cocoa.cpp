// CAdManager_Cocoa.cpp - iOS ICAdManager; every call goes to the AdMarvel singleton (type 0 only).
#include "CAdManager_Cocoa.h"

#include "CAdManager_AdMarvel_Cocoa.h"

ICAdManager* ICAdManager::GetInstance()
{
    void* found = NULL;
    CApplet::m_pApp->m_singletons->Find(ClassId, &found);
    ICAdManager* instance = (ICAdManager*)found;
    if (instance == NULL) {
        instance = new CAdManager_Cocoa();
    }
    return instance;
}

void CAdManager_Cocoa::Init(int phoneSiteId, int tabletSiteId)
{
    m_adMarvel = CAdManager_AdMarvel_Cocoa::GetInstance();
    if (m_adMarvel != NULL) {
        m_adMarvel->Init(phoneSiteId, tabletSiteId);
    }
}

void CAdManager_Cocoa::Destroy()
{
    if (m_adMarvel != NULL) {
        m_adMarvel->Destroy();
        m_adMarvel = NULL;
    }
}

void CAdManager_Cocoa::ShowAd(Type type, Position position, int x, int y, Callback callback, void* context)
{
    if (type == Type_AdMarvel) {
        // The original drops the callback and its context
        m_adMarvel->ShowAd(position, x, y, NULL, NULL);
    }
}

unsigned char CAdManager_Cocoa::IsAvailable(Type type)
{
    unsigned char available = 0;
    if (type == Type_AdMarvel) {
        available = m_adMarvel->IsAvailable();
    }
    return available;
}

unsigned char CAdManager_Cocoa::IsVisible()
{
    unsigned char visible = 0;
    if (m_adMarvel->IsAvailable()) {
        visible = m_adMarvel->IsVisible();
    }
    return visible;
}

unsigned char CAdManager_Cocoa::IsFullScreen()
{
    unsigned char fullScreen = 0;
    if (m_adMarvel->IsAvailable()) {
        fullScreen = m_adMarvel->IsFullScreen();
    }
    return fullScreen;
}

void CAdManager_Cocoa::HideAd()
{
    if (m_adMarvel->IsAvailable()) {
        m_adMarvel->HideAd();
    }
}

void CAdManager_Cocoa::HandleUpdate(int deltaMilliseconds)
{
    if (m_adMarvel->IsAvailable()) {
        m_adMarvel->HandleUpdate(deltaMilliseconds);
    }
}

void CAdManager_Cocoa::HandleRender()
{
    if (m_adMarvel->IsAvailable()) {
        m_adMarvel->HandleRender();
    }
}

CAdManager_Cocoa::~CAdManager_Cocoa()
{
}
