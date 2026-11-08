// CLoginSteps.cpp - login steps of the game's gServe login flow.
#include "AdBannerSizes.h"
#include "CLoginSteps.h"
#include "WindowApp.h"
#include "CNGSOfferManager.h"

void VersionCheckStep::OnEvent(eLoginFlowEvent event, const char* caller)
{
    if (event == LOGIN_FLOW_EVENT_7) {
        // version invalid: let the game show the update prompt
        WindowApp::HandleTunnelCommand(0x6458fa9c, 0, 0, 0);
    } else {
        CNGSLoginFlow::GetInstance()->NextStep();
    }
}

// Reconstructed (dead-stripped in the shipped binary): CLoginSteps.o keeps the store of
// ClassId_COfferManager in its static initializer, so some function of this file read it (most
// likely another login step using COfferManager::GetInstance()). Name and body are guesses.
COfferManager* LoginSteps_GetOfferManager()
{
    return COfferManager::GetInstance();
}
