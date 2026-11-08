// ICInAppPurchase.h - platform-independent in-app purchase service (created by b033x).
// A singleton (key ClassId_ICInAppPurchase); the platform implementation is CInAppPurchase_Cocoa.
#ifndef ICINAPPPURCHASE_H
#define ICINAPPPURCHASE_H

#include "CSingleton.h"
#include "CStringToKey.h"
#include "CStrWChar.h"
#include "CVector.h"
#include "ICInAppPurchaseDelegate.h"
#include "ICInAppPurchaseProduct.h"
#include "ICInAppPurchaseTransaction.h"

/** Every includer computes the class id at static initialization (one __GLOBAL__I_a per unit in the original). */
static unsigned int ClassId_ICInAppPurchase = com::glu::platform::core::CStringToKey("ICInAppPurchase", 0);

class ICInAppPurchase : public com::glu::platform::components::CSingleton {
public:
    ICInAppPurchase()
        : CSingleton(ClassId_ICInAppPurchase)
    {
    }

    virtual ~ICInAppPurchase()
    {
    }

    /** Create the platform implementation (defined in CInAppPurchase_Cocoa.mm). */
    static ICInAppPurchase* CreateInstance();

    /** The store of the running application, created on first use (not cached). */  // added by b057x
    static inline ICInAppPurchase* GetInstance()  // added by b057x
    {  // added by b057x
        void* found = NULL;  // added by b057x
        CApplet::m_pApp->m_singletons->Find(ClassId_ICInAppPurchase, &found);  // added by b057x
        ICInAppPurchase* instance = (ICInAppPurchase*)found;  // added by b057x
        if (instance == NULL) {  // added by b057x
            instance = CreateInstance();  // added by b057x
        }  // added by b057x
        return instance;  // added by b057x
    }  // added by b057x

    /** Start observing the store; results go to `delegate`. */
    virtual void Init(ICInAppPurchaseDelegate* delegate) = 0;
    /** Restore completed transactions (SyncTransactionsComplete). */
    virtual void SyncTransactions() = 0;
    /** Query the products `productIds`; `timeout` > 0 cancels the request after that many ms of HandleUpdate time. */
    virtual void RequestProductData(const com::glu::platform::core::CVector<com::glu::platform::components::CStrWChar>& productIds,
                                    int timeout) = 0;
    virtual void BuyProduct(const com::glu::platform::components::CStrWChar& productId) = 0;
    virtual unsigned char IsTransactionActive() const = 0;
    virtual unsigned char IsAvailable() const = 0;
    virtual unsigned char AllowsConcurrentOperations() const = 0;
    virtual void HandleUpdate(int deltaTime) = 0;
};

#endif
