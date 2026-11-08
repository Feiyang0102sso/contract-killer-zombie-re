// ICInAppPurchaseDelegate.h - callbacks of ICInAppPurchase (created by b033x; implemented by CGameApp).
#ifndef ICINAPPPURCHASEDELEGATE_H
#define ICINAPPPURCHASEDELEGATE_H

#include "CVector.h"

class ICInAppPurchaseProduct;
class ICInAppPurchaseTransaction;

/** Receives the results of the asynchronous ICInAppPurchase operations. No virtual destructor (slot 0 is SyncTransactionsComplete). */
class ICInAppPurchaseDelegate {
public:
    enum SyncTransactionsState {
        SYNC_TRANSACTIONS_SUCCESS = 0,
        SYNC_TRANSACTIONS_FAILED = 1
    };

    enum RequestProductDataState {
        REQUEST_PRODUCT_DATA_SUCCESS = 0,
        /** The request timed out or was cancelled (no NSError). */
        REQUEST_PRODUCT_DATA_TIMEOUT = 1,
        /** The store reported an error. */
        REQUEST_PRODUCT_DATA_FAILED = 2
    };

    enum BuyProductState {
        BUY_PRODUCT_SUCCESS = 0,
        /** SKErrorPaymentCancelled. */
        BUY_PRODUCT_CANCELLED = 1,
        /** Not produced by the Cocoa implementation. */
        BUY_PRODUCT_UNKNOWN = 2,
        BUY_PRODUCT_FAILED = 3
    };

    /** Restored transactions (deleted by the caller after the call). */
    virtual void SyncTransactionsComplete(SyncTransactionsState state,
                                          const com::glu::platform::core::CVector<const ICInAppPurchaseTransaction*>& transactions) = 0;
    /** Valid products of the last request (deleted by the caller after the call). */
    virtual void RequestProductDataComplete(RequestProductDataState state,
                                            const com::glu::platform::core::CVector<const ICInAppPurchaseProduct*>& products) = 0;
    virtual void BuyProductComplete(BuyProductState state, const ICInAppPurchaseTransaction* transaction) = 0;
};

#endif
