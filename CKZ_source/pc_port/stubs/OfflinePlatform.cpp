// The Windows campaign runs without the original iOS online services.
#include "CStrWChar.h"
#include "CNetworkAvailability.h"
#include "ICNotification.h"
#include "ICInAppPurchase.h"
#include "ICSocket.h"
#include "LocalPushManager.h"
#include "GameCenter_Facade.h"

using namespace com::glu::platform;
using namespace com::glu::platform::components;

namespace {

class OfflineSocket final : public network::ICSocket {
public:
    unsigned char IsBusy() const override { return 0; }
    int GetError() const override { return Error_NotConnected; }
    int GetSocketError() const override { return 0; }
    unsigned char* GetDataPtr() const override { return nullptr; }
    unsigned int GetDataLen() const override { return 0; }
    int Connect(eProtocol, const char*, unsigned short, int, unsigned char) override { return Error_NotConnected; }
    void Disconnect() override {}
    int Send(const unsigned char*, unsigned int, int) override { return Error_NotConnected; }
    int Receive(unsigned char*, unsigned int, int) override { return Error_NotConnected; }
    void HandleUpdate(int) override {}
};

class OfflineNotification final : public ICNotification {
public:
    unsigned char Register(Type, const wchar_t*, const wchar_t*, void*, Callback) override { return 0; }
    unsigned char Register(Type, unsigned int, unsigned int, const wchar_t*, const wchar_t*, void*, Callback) override { return 0; }
    unsigned char Cancel() override { return 0; }
    unsigned char IsEnabled() override { return 0; }
    unsigned char GetIdentity(CStrWChar&) override { return 0; }
    unsigned char SetBadgeNumber(unsigned int) override { return 0; }
    unsigned char GetBadgeNumber(unsigned int&) override { return 0; }
};

class OfflineStore final : public ICInAppPurchase {
public:
    void Init(ICInAppPurchaseDelegate*) override {}
    void SyncTransactions() override {}
    void RequestProductData(const core::CVector<CStrWChar>&, int) override {}
    void BuyProduct(const CStrWChar&) override {}
    unsigned char IsTransactionActive() const override { return 0; }
    unsigned char IsAvailable() const override { return 0; }
    unsigned char AllowsConcurrentOperations() const override { return 0; }
    void HandleUpdate(int) override {}
};

} // namespace

network::ICSocket* network::ICSocket::CreateInstance() { return new OfflineSocket; }
ICNotification* ICNotification::CreateInstance() { return new OfflineNotification; }
ICInAppPurchase* ICInAppPurchase::CreateInstance() { return new OfflineStore; }

CNetworkAvailability::CNetworkAvailability() : m_reachability(nullptr) {}
CNetworkAvailability::~CNetworkAvailability() = default;
unsigned char CNetworkAvailability::IsAvailable() const { return 0; }

void LocalPushManager::CancelLocalPushNotifications(bool) {}
void LocalPushManager::SetupLocalPush(const char*, const char*, int, int) {}
void LocalPushManager::SetLocalPushNotification(const char*, const char*, int, int) {}

GameCenter_Facade::GameCenter_Facade() : CSingleton(ClassId), m_authenticationState(RequestState_Failed) {}
GameCenter_Facade::~GameCenter_Facade() = default;
bool GameCenter_Facade::isAuthenticated() { return false; }
void GameCenter_Facade::AuthenticatePlayer() {}
bool GameCenter_Facade::isGameCenterSupported() { return false; }
void GameCenter_Facade::StartPostingScore(const char*, int) {}
void GameCenter_Facade::StartLoadingLeaderboards(GameCenter_LeaderboardFilter*, int) {}
void GameCenter_Facade::StartPostingAchievement(const char*, float) {}
void GameCenter_Facade::FetchPosting() {}
int GameCenter_Facade::GetLeaderboardSize(const char*) { return 0; }
void GameCenter_Facade::GetLeaderboardData(const char*, int, GameCenter_LeaderboardData&) {}
void GameCenter_Facade::GetLocalPLayerName(CStrWChar& name) { name = L"PLAYER"; }
void GameCenter_Facade::GetLocalPlayerData(const char*, GameCenter_LeaderboardData&) {}
void GameCenter_Facade::RunLeaderboardsView(const char*) {}
