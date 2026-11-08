// CInAppPurchaseTransaction_Cocoa.h - ICInAppPurchaseTransaction backed by a retained SKPaymentTransaction (created by b033x).
#ifndef CINAPPPURCHASETRANSACTION_COCOA_H
#define CINAPPPURCHASETRANSACTION_COCOA_H

#include "ICInAppPurchaseTransaction.h"

#ifdef __OBJC__
@class SKPaymentTransaction;
#endif

class CInAppPurchaseTransaction_Cocoa : public ICInAppPurchaseTransaction {
public:
    /** Retains `transaction`. */
    CInAppPurchaseTransaction_Cocoa(const SKPaymentTransaction* transaction);
    virtual ~CInAppPurchaseTransaction_Cocoa();

    virtual int GetTransactionState() const;
    virtual com::glu::platform::components::CStrWChar GetProductIdentifier() const;
    virtual com::glu::platform::components::CStrWChar GetTransactionIdentifier() const;
    virtual com::glu::platform::components::CStrWChar GetErrorDescription() const;
    virtual com::glu::platform::components::CStrWChar GetReceiptDescription() const;
    virtual com::glu::platform::components::CStrWChar GetDate() const;

private:
    const SKPaymentTransaction* m_transaction;  // 0x04 retained
};

#endif
