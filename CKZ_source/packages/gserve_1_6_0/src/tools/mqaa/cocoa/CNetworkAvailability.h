// Owner: tools/mqaa/cocoa/CNetworkAvailability.mm (b028x; started as a PROVISIONAL)
// CNetworkAvailability.h - wraps the iOS reachability check used by the gServe message queue.
#ifndef CNETWORKAVAILABILITY_H
#define CNETWORKAVAILABILITY_H

class CNetworkAvailability {
public:
    CNetworkAvailability();
    ~CNetworkAvailability();

    /** An internet connection is currently reachable. */
    unsigned char IsAvailable() const;

    void* m_reachability;   // +0x00 Reachability object (Objective-C); object size 4
};

#endif
