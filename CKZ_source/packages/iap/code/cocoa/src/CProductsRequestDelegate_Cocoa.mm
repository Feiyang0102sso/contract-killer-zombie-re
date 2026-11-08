// CProductsRequestDelegate_Cocoa.mm - forwards SKProductsRequest results to CInAppPurchase_Cocoa.
#include "CProductsRequestDelegate_Cocoa.h"

#include "CInAppPurchase_Cocoa.h"

@implementation CProductsRequestDelegate_Cocoa

- (id)init
{
    return [self initWithIAP:NULL];
}

- (id)initWithIAP:(CInAppPurchase_Cocoa*)iap
{
    self = [super init];
    if (self) {
        m_iap = iap;
    }
    return self;
}

- (void)productsRequest:(SKProductsRequest*)request didReceiveResponse:(SKProductsResponse*)response
{
    NSArray* products = [response products];
    NSUInteger productCount = [products count];
    NSArray* invalidIdentifiers = [response invalidProductIdentifiers];
    NSUInteger invalidCount = [invalidIdentifiers count];

    for (NSUInteger index = 0; index < productCount; index++) {
        m_iap->ValidProduct(request, [products objectAtIndex:index]);
    }
    for (NSUInteger index = 0; index < invalidCount; index++) {
        m_iap->InvalidProduct(request, [invalidIdentifiers objectAtIndex:index]);
    }
}

- (void)request:(SKRequest*)request didFailWithError:(NSError*)error
{
    m_iap->RequestFailed((SKProductsRequest*)request, error);
    [request autorelease];
}

- (void)requestDidFinish:(SKRequest*)request
{
    m_iap->RequestComplete((SKProductsRequest*)request);
    [request autorelease];
}

- (void)dealloc
{
    m_iap = NULL;
    [super dealloc];
}

@end
