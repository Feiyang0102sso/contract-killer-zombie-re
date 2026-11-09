// Disabled iOS advertising and analytics services.
#include "CFlurry.h"
#include "CPHInterface.h"
#include "ICAdManager.h"
#include "TapjoyInterface.h"

namespace {

class OfflineAds final : public ICAdManager {
public:
    void Init(int, int) override {}
    void Destroy() override {}
    void ShowAd(Type, Position, int, int, Callback, void*) override {}
    unsigned char IsAvailable(Type) override { return 0; }
    unsigned char IsVisible() override { return 0; }
    unsigned char IsFullScreen() override { return 0; }
    void HideAd() override {}
    void HandleUpdate(int) override {}
    void HandleRender() override {}
};

} // namespace

ICAdManager* ICAdManager::GetInstance()
{
    void* found = nullptr;
    CApplet::m_pApp->m_singletons->Find(ClassId, &found);
    if (found != nullptr) {
        return static_cast<ICAdManager*>(found);
    }
    return new OfflineAds;
}

void CFlurry::StartSession(const char*) {}
void CFlurry::LogCustomEvent(const char*, const wchar_t*, int, int) {}
void CFlurry::LogCustomEvent(const char*, const char*, int, int) {}
void CFlurry::SetSessionReportsOnCloseEnabled(bool) {}
void CFlurry::SetSessionReportsOnPauseEnabled(bool) {}

void CPHInterface::init(const char*, const char*) {}
void CPHInterface::destroy() {}
void CPHInterface::startPublisherContentRequest(const char*, bool, bool) {}
bool CPHInterface::isPublisherContentRequestActive(const char*) { return false; }
void CPHInterface::showNotificationView(const char*, bool, unsigned int, unsigned int) {}
void CPHInterface::hideNotificationView(const char*) {}

void TapjoyInterface::initialize(const char*, const char*, const char*, bool) {}
void TapjoyInterface::destroy() {}
bool TapjoyInterface::getFeaturedAppQueryState() { return false; }
TapjoyInterface::FeaturedAppInfo* TapjoyInterface::getFeaturedApp() { return nullptr; }
void TapjoyInterface::openTapjoyInterface() {}
unsigned char TapjoyInterface::canDisplayInterface() { return 0; }
unsigned int TapjoyInterface::getRemainingTapjoyPoints() { return 0; }
unsigned char TapjoyInterface::consumeTapjoyPoints(unsigned int) { return 0; }
unsigned int TapjoyInterface::convertPointsToGameCurrency(unsigned int) { return 0; }
void TapjoyInterface::onResume() {}
void TapjoyInterface::registerVideoCallback(void (*)(unsigned char, void*), void*) {}
