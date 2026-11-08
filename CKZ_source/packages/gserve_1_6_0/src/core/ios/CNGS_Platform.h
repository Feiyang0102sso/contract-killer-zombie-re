// CNGS_Platform.h - iOS services of gServe: the Game Center and Facebook interfaces and the
// native dialogs (prompt messages, text entry, alerts). Implemented in CNGS_Platform.mm.
#ifndef CNGS_PLATFORM_H
#define CNGS_PLATFORM_H

#include <stddef.h>
#include "CSingleton.h"
#include "CStrWChar.h"
#include "CVector.h"

namespace com { namespace glu { namespace platform { namespace math {
class CVector2d;
} } } }

class CNGSMessagePrompt;

/**
 * Receiver of the text entry dialog result (layout from the ObjC type encoding {CDialogFunctor=^^?^v}).
 * The dialog deletes it when it closes.
 */
class CDialogFunctor {
public:
    explicit CDialogFunctor(void* context)
        : m_context(context)
    {
    }

    virtual ~CDialogFunctor() {}

    /** A button was pressed: its tag (1 = OK, 0 = Cancel). */
    virtual void buttonPressed(int buttonTag) = 0;

    void* m_context;    // +0x04
};
class CNGSSocialInterface;
class CGameCenterInterface;
class CFacebookInterface;

/** Social networks served by getSocialInterface(). */
enum eSocialInterfaceType {
    SOCIAL_INTERFACE_GAME_CENTER = 1,
    SOCIAL_INTERFACE_FACEBOOK = 2
};

class CNGS_Platform : public com::glu::platform::components::CSingleton {
public:
    /** Singleton key in CApp::m_singletons. */
    enum { ClassId = 0xeaf5aa27 };

    CNGS_Platform();
    virtual ~CNGS_Platform();

    /** The registered instance, created on first use. */
    static inline CNGS_Platform* GetInstance()
    {
        void* found = NULL;
        CApplet::m_pApp->m_singletons->Find(ClassId, &found);
        CNGS_Platform* instance = (CNGS_Platform*)found;
        if (instance == NULL) {
            instance = new CNGS_Platform();
        }
        return instance;
    }

    /** Show `message` with the prompt's buttons (`buttonLabels`: cancel, ok) when `hasButtons`. */
    virtual void showPromptMessageDialog(com::glu::platform::components::CStrWChar& message, CNGSMessagePrompt* prompt,
        com::glu::platform::core::CVector<const com::glu::platform::components::CStrWChar*>* buttonLabels,
        unsigned char hasButtons);
    /** Ask for a line of text; the result is written to *text and `functor` is called. */
    virtual void showGenericTextEntryDialog(com::glu::platform::components::CStrWChar& message, CDialogFunctor* functor,
        const com::glu::platform::math::CVector2d& textFieldSize, wchar_t** text);
    virtual void showGenericAlertPopup(const com::glu::platform::components::CStrWChar& message);

    virtual unsigned char isGameCenterAvailable()
    {
        return m_gameCenterSupported;
    }

    /** The Game Center (when supported) or Facebook interface, NULL otherwise. */
    virtual CNGSSocialInterface* getSocialInterface(int type);

    /** Create the social interfaces. */
    void init();
    /** Delete the social interfaces. */
    void destroy();
    /** GKLocalPlayer exists, iOS >= 4.1 and not a first generation iPhone. */
    static unsigned char isGameCenterSupported();

    CGameCenterInterface* m_gameCenterInterface;    // +0x08 owned
    CFacebookInterface* m_facebookInterface;        // +0x0c owned
    int m_reserved10;                               // +0x10 0 after init()
    unsigned char m_gameCenterSupported;            // +0x14
};

#endif
