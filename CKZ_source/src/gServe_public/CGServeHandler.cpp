// CGServeHandler.cpp - game side helpers around gServe (Facebook setup, social network dialogs,
// taking over the server's profile data).
#include "AdBannerSizes.h"
#include "CGServeHandler.h"
#include "CNGSOfferManager.h"
#include "ICInAppPurchase.h"
#include "CNGS_Platform.h"
#include "CNGSSocialInterface.h"
#include "CProfileManager.h"
#include "CStrWChar.h"
#include "App.h"
#include "AppSettings.h"
#include "CZombieDialogWindow.h"
#include "CPrivateData1.h"

using namespace com::glu::platform::components;
using namespace com::glu::platform::core;

/** Facebook application of the game. */
static const wchar_t FACEBOOK_APP_ID[] = L"232699846762828";
/** The secret is not shipped with the client. */
static const wchar_t FACEBOOK_API_SECRET[] = L"";

CGServeHandler::CGServeHandler()
{
}

CGServeHandler::~CGServeHandler()
{
}

void CGServeHandler::Init()
{
    m_field10 = 0;
    CNGSSocialInterface* facebook = CNGS_Platform::GetInstance()->getSocialInterface(2);
    facebook->setAppID(CStrWChar(FACEBOOK_APP_ID), CStrWChar(FACEBOOK_API_SECRET));
    m_field08 = 0;
    m_field0c = 0;
}

void CGServeHandler::HandleUpdate()
{
}

unsigned char CGServeHandler::CloseCurrentSocialNetworkConnectDialog()
{
    unsigned char closed = 0;
    Window* dialog = ((App*)WindowApp::m_instance)->m_socialConnectDialog;  // changed by fx2
    ((App*)WindowApp::m_instance)->m_field3c0 = -1;
    if (dialog != NULL) {
        dialog->CloseSafely();
        ((App*)WindowApp::m_instance)->m_socialConnectDialog = NULL;  // changed by fx2
        closed = 1;
    }
    return closed;
}

void CGServeHandler::ShowSocialNetworkSessionInvalidDialog()
{
    CloseCurrentSocialNetworkConnectDialog();
    CZombieDialogWindow* dialog = new CZombieDialogWindow(3);
    dialog->AddButton(CZombieDialogWindow::BUTTON_OK, 0x42650932, 0, 0, true, 0x14);
    dialog->SetText(Window::ResString("IDS_SESSION_INVALID"), false, NULL);
    ((App*)WindowApp::m_instance)->m_mainLayer->AddModal(dialog);
}

void CGServeHandler::SelectProfileServerData(const CVector<int>& collections)
{
    App* app = (App*)WindowApp::m_instance;
    AppSettings* settings = app->m_settings;
    app->m_profileManager->useDataFromServer(collections);  // changed by fx2
    settings->SetSoundEnabled(app->m_privateData->getSoundSetting() != 0);
    settings->SetMusicEnabled(app->m_privateData->getMusicSetting() != 0);
    settings->SetFireButtonEnabled(app->m_privateData->getFireBtnSetting() != 0);
    settings->SetLocalPushEnabled(app->m_privateData->getNotificationSetting() != 0);
    settings->Save();
}
