// CInAppPurchase_Cocoa.h - ICInAppPurchase on StoreKit (created by b033x).
// Product requests go through CProductsRequestDelegate_Cocoa, payments through CTransactionObserver_Cocoa;
// both call back into the public non-virtual handlers below.
#ifndef CINAPPPURCHASE_COCOA_H
#define CINAPPPURCHASE_COCOA_H

#include "ICInAppPurchase.h"

#ifdef __OBJC__
@class NSError;
@class NSString;
@class SKPaymentTransaction;
@class SKProduct;
@class SKProductsRequest;
@class CProductsRequestDelegate_Cocoa;
@class CTransactionObserver_Cocoa;
#endif

class CInAppPurchase_Cocoa : public ICInAppPurchase {
public:
    /** Value of m_requestTimeout while no timed request is pending. */
    static const int NoRequestTimeout = -2;

    CInAppPurchase_Cocoa();
    virtual ~CInAppPurchase_Cocoa();

    virtual void Init(ICInAppPurchaseDelegate* delegate);
    virtual void SyncTransactions();
    virtual void RequestProductData(const com::glu::platform::core::CVector<com::glu::platform::components::CStrWChar>& productIds,
                                    int timeout);
    virtual void BuyProduct(const com::glu::platform::components::CStrWChar& productId);
    virtual unsigned char IsTransactionActive() const;
    virtual unsigned char IsAvailable() const;
    virtual unsigned char AllowsConcurrentOperations() const;
    virtual void HandleUpdate(int deltaTime);

    // SKProductsRequestDelegate side
    void RequestFailed(SKProductsRequest* request, NSError* error);
    void ValidProduct(const SKProductsRequest* request, SKProduct* product);
    void InvalidProduct(const SKProductsRequest* request, const NSString* productId);
    void RequestComplete(SKProductsRequest* request);

    // SKPaymentTransactionObserver side
    void UpdatedTransaction(SKPaymentTransaction* transaction);
    void RestoreFailed(const NSError* error);
    void RestoreCompleted();

private:
    CTransactionObserver_Cocoa* m_transactionObserver;              // 0x08 owned, registered with the payment queue
    CProductsRequestDelegate_Cocoa* m_productsRequestDelegate;      // 0x0c owned
    unsigned int m_pendingOperations;                               // 0x10 store operations awaiting a callback (b142: ObjC type encoding is I)
    ICInAppPurchaseDelegate* m_delegate;                            // 0x14
    com::glu::platform::core::CVector<SKPaymentTransaction*> m_restoredTransactions;  // 0x18 retained until RestoreCompleted
    int m_requestTimeout;                                           // 0x30 ms, NoRequestTimeout when idle
    int m_requestElapsed;                                           // 0x34 ms since the request started
    unsigned char m_resetRequestTimer;                              // 0x38 restart m_requestElapsed on the next update
    unsigned char m_requestTimerActive;                             // 0x39
    SKProductsRequest* m_productsRequest;                           // 0x3c request in flight (released by the delegate)
    com::glu::platform::core::CVector<SKProduct*> m_products;       // 0x40 retained valid products of the request
};

#endif
