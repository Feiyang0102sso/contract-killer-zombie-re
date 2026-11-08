// CGServeHandler.h - game side helpers around gServe: Facebook app id setup, the social network
// dialogs and taking over the server's profile data. (created by b057x, completed by b047x)
#ifndef CGSERVEHANDLER_H
#define CGSERVEHANDLER_H

#include <stddef.h>
#include "CVector.h"

class CGServeHandler {
public:
    CGServeHandler();
    virtual ~CGServeHandler();

    /** Give the Facebook interface the app id / secret and reset the state. */
    void Init();
    void HandleUpdate();
    /** Close the open social network connect dialog; 1 when one was open. */
    unsigned char CloseCurrentSocialNetworkConnectDialog();
    /** Close the connect dialog and tell the user the social network session became invalid. */
    void ShowSocialNetworkSessionInvalidDialog();
    /** Take the server's copy of `collections` and apply its settings. */
    void SelectProfileServerData(const com::glu::platform::core::CVector<int>& collections);

    int m_field04;                  // 0x04 not used by this module
    int m_field08;                  // 0x08 cleared by Init
    int m_field0c;                  // 0x0c cleared by Init
    unsigned char m_field10;        // 0x10 cleared by Init
};

#endif
