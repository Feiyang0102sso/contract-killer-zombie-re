// ICInAppPurchaseTransaction.h - one payment transaction reported by the store (created by b033x).
#ifndef ICINAPPPURCHASETRANSACTION_H
#define ICINAPPPURCHASETRANSACTION_H

#include "CStrWChar.h"

/** Read-only view of a payment transaction (platform implementation: CInAppPurchaseTransaction_Cocoa). */
class ICInAppPurchaseTransaction {
public:
    /** Same values as SKPaymentTransactionState. */
    enum TransactionState {
        TRANSACTION_STATE_PURCHASING = 0,
        TRANSACTION_STATE_PURCHASED = 1,
        TRANSACTION_STATE_FAILED = 2,
        TRANSACTION_STATE_RESTORED = 3
    };

    virtual ~ICInAppPurchaseTransaction() {}

    virtual int GetTransactionState() const = 0;
    virtual com::glu::platform::components::CStrWChar GetProductIdentifier() const = 0;
    /** Empty unless the transaction was purchased or restored. */
    virtual com::glu::platform::components::CStrWChar GetTransactionIdentifier() const = 0;
    /** Empty unless the transaction failed. */
    virtual com::glu::platform::components::CStrWChar GetErrorDescription() const = 0;
    /** Empty unless the transaction was purchased. */
    virtual com::glu::platform::components::CStrWChar GetReceiptDescription() const = 0;
    /** Empty unless the transaction was purchased or restored. */
    virtual com::glu::platform::components::CStrWChar GetDate() const = 0;
};

#endif
