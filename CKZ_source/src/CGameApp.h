// b063: owner src/CGameApp.cpp is fully recovered; the old PROVISIONAL comments below are kept as history.
// Original ctor, callback thunks and vtable prove: CApp 0x50, IAP delegate +0x50, total size 0xa8.
// PROVISIONAL (created by b057x): owner src/CGameApp.cpp not written yet
// CGameApp.h - the game application (node tree root); only what src/general uses.
#ifndef CGAMEAPP_H
#define CGAMEAPP_H

#include "ICInAppPurchase.h"
#include "CStrWChar.h"
#include "CApp.h"
#include "ICNotification.h"
#include "ICDisplayProgram.h"
#include "Vector_ss.h"
#include "xString.h"

namespace com { namespace glu { namespace platform { namespace systems {
class CEvent;
class CEventListener;
} } } }
class CNode;

/** Source of the push notification token (vtable slot 0x18). */
// b063: this earlier CPushTokenSource declaration is the real ICNotification interface.
/** The device token (empty when none was received). */

/** The part of CGameApp in front of its purchase delegate (layout not known yet). */
// b063: CGameAppBase is the actual framework::CApp (0x50), including its cached interfaces.

class CGameApp : public com::glu::platform::framework::CApp, public ICInAppPurchaseDelegate {
public:
    CGameApp();
    virtual ~CGameApp();
    static CGameApp* GetInstance();

    void HandleUpdate();
    void HandleRender();
    static unsigned char EventCB(com::glu::platform::systems::CEvent& event, void* listener);
    unsigned char HandleEvent(unsigned int id, unsigned int parameter1, unsigned int parameter2);
    virtual int OnInit();
    virtual void OnFree();
    virtual void OnTimer();
    virtual void OnSystemEvent(unsigned int id, unsigned int parameter1, unsigned int parameter2);
    void CreatePools() const;
    void DestroyPools() const;
    int InitRegistry();
    void ReleaseRegistry();
    static void OnTapjoyVideo(unsigned char state, void* context);
    void ReInit();
    void HandlePushNotification(const wchar_t* text);
    void OnIAPComplete(const XString& productId);
    void HandleIAPPurchase(int cash, int credits);
    virtual void SyncTransactionsComplete(SyncTransactionsState state,
        const com::glu::platform::core::CVector<const ICInAppPurchaseTransaction*>& transactions);
    virtual void RequestProductDataComplete(RequestProductDataState state,
        const com::glu::platform::core::CVector<const ICInAppPurchaseProduct*>& products);
    virtual void BuyProductComplete(BuyProductState state, const ICInAppPurchaseTransaction* transaction);

    /** Set when the local and the server profile data conflict (CNotificationHandler). */  // added by b047x
    static bool gserveDataConflict;  // added by b047x
    static bool syncServerFlag;

    // Former m_data54[0xa4 - 0x54]: fields recovered from ctor, callbacks and rendering.
    com::glu::platform::systems::CEventListener* m_eventListener;  // 0x54
    CNode* m_rootNode;                                          // 0x58 AppNode
    Vector<XString> m_pendingPurchases;                         // 0x5c
    Vector<XString> m_pendingNotifications;                     // 0x6c
    unsigned char m_renderEnabled;                             // 0x7c
    com::glu::platform::graphics::ICDisplayProgram* m_displayProgram; // 0x80
    com::glu::platform::graphics::ICDisplayProgram::Mode m_displayMode; // 0x84
    com::glu::platform::components::ICNotification* m_pushTokenSource; // 0xa4
};

#endif
