// CNetworkAvailability.mm - internet reachability check for the gServe message queue.
#include "CNetworkAvailability.h"
#import "Reachability.h"

CNetworkAvailability::CNetworkAvailability()
{
    m_reachability = NULL;
    Reachability* reachability = [[Reachability reachabilityForInternetConnection] retain];
    [reachability startNotifier];
    m_reachability = reachability;
}

CNetworkAvailability::~CNetworkAvailability()
{
    [(Reachability*)m_reachability release];
    m_reachability = NULL;
}

unsigned char CNetworkAvailability::IsAvailable() const
{
    return [(Reachability*)m_reachability currentReachabilityStatus] != NotReachable;
}
