// CNGSOfferDelegate.h - steps of an offer that run on the client before the "offer selected"
// request: posting invites to friends (InviteDelegate) or the featured game (FeaturedGameDelegate).
// Each delegate reports back with COfferManager::finishedExecuting().
#ifndef CNGSOFFERDELEGATE_H
#define CNGSOFFERDELEGATE_H

#include <stddef.h>
#include "CStrWChar.h"

class COfferDataSetSelected;
class CInviteOffer;

/** Common part of the delegates (no constructor or destructor of its own). */
class COfferDelegate {
public:
    /** m_state values. */
    enum {
        STATE_IDLE = 0,
        STATE_READY = 1,
        STATE_RUNNING = 3,
        STATE_POSTING = 4
    };

    /** Start the step; 0 when started, otherwise an error code (3 busy, 6 nothing to do). */
    virtual int execute() = 0;

    COfferDataSetSelected* m_selectedData;  // +0x04 payload of the "offer selected" request, owned
    int m_state;                            // +0x08
};

/** Posts one message to a social network and waits for handlePostMessage(). */
class PostMessageDelegate : public COfferDelegate {
public:
    PostMessageDelegate();

    /** Post the message to m_recipientId on network m_networkType (once at a time). */
    void PostMessage();
    /** The post finished. */
    virtual void handlePostMessage(unsigned char success);

    com::glu::platform::components::CStrWChar m_message;       // +0x0c
    com::glu::platform::components::CStrWChar m_link;          // +0x1c
    com::glu::platform::components::CStrWChar m_description;   // +0x2c
    com::glu::platform::components::CStrWChar m_picture;       // +0x3c
    com::glu::platform::components::CStrWChar m_name;          // +0x4c
    com::glu::platform::components::CStrWChar m_caption;       // +0x5c
    com::glu::platform::components::CStrWChar m_privacy;       // +0x6c
    com::glu::platform::components::CStrWChar m_recipientId;   // +0x7c friend the message goes to
    int m_networkType;                                          // +0x8c social network, -1 by default
    unsigned char m_posting;                                    // +0x90 a post is in flight
};

/** Invites every potential invitee of the offer, one post after the other. */
class InviteDelegate : public PostMessageDelegate {
public:
    InviteDelegate();
    ~InviteDelegate();

    virtual int execute();
    virtual void handlePostMessage(unsigned char success);

    /** Post to the next potential invitee, or report the end to COfferManager. */
    void sendNextInvite();

    CInviteOffer* m_offer;          // +0x94 offer being executed (not owned)
    int m_invitesRequested;         // +0x98 execute() needs at least 1
    int m_invitesSent;              // +0x9c
};

/** Featured game offer: nothing to post, reports success at once. */
class FeaturedGameDelegate : public COfferDelegate {
public:
    FeaturedGameDelegate();
    ~FeaturedGameDelegate();

    virtual int execute();

    unsigned char m_busy;           // +0x0c
    int m_value10;                  // +0x10 meaning unknown, 0
};

#endif
