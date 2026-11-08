// TapjoyInterface.mm - C++ entry points of the Tapjoy integration; everything forwards to TapjoyManager.
#include "TapjoyInterface.h"

#import "TapjoyManager.h"

TapjoyManager* TapjoyInterface::m_pTapjoy;
TapjoyInterface::FeaturedAppInfo* TapjoyInterface::m_pFeaturedAppInfo;
bool TapjoyInterface::m_FeaturedAppQueryInProgress;

void TapjoyInterface::initialize(const char* appId, const char* currencyId, const char* secretKey, bool enableVideoAds)
{
    m_pTapjoy = [[TapjoyManager alloc] init:appId :secretKey :enableVideoAds];
    m_pFeaturedAppInfo = NULL;
}

void TapjoyInterface::destroy()
{
    [m_pTapjoy release];
    m_pTapjoy = nil;
    delete m_pFeaturedAppInfo;
}

bool TapjoyInterface::getFeaturedAppQueryState()
{
    return m_FeaturedAppQueryInProgress;
}

TapjoyInterface::FeaturedAppInfo* TapjoyInterface::getFeaturedApp()
{
    return m_pFeaturedAppInfo;
}

void TapjoyInterface::openTapjoyInterface()
{
    [m_pTapjoy displayInterface];
}

unsigned char TapjoyInterface::canDisplayInterface()
{
    return [m_pTapjoy canDisplayInterface];
}

unsigned int TapjoyInterface::getRemainingTapjoyPoints()
{
    return [m_pTapjoy getRemainingTapjoyPoints];
}

unsigned char TapjoyInterface::consumeTapjoyPoints(unsigned int points)
{
    return [m_pTapjoy consumeTapjoyPoints:points];
}

unsigned int TapjoyInterface::convertPointsToGameCurrency(unsigned int points)
{
    return [m_pTapjoy convertPointsToGameCurrency:points];
}

void TapjoyInterface::onResume()
{
    [m_pTapjoy onResume];
}

void TapjoyInterface::registerVideoCallback(void (*callback)(unsigned char, void*), void* context)
{
    [m_pTapjoy registerVideoCallback:callback :context];
}
