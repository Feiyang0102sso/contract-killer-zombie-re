// YesNoDialog.h - question dialogs with "yes" / "no" items, the first start sound question
// (with a hidden cheat code input) and a message dialog with "OK".
// Owner: src/general/YesNoDialog.cpp (b058; first draft by b057x).
// Slot order from the vtables of YesNoDialog / GameStartSoundDialog(::CheatsWindow) / SimpleMessage,
// fields from their constructors.
#ifndef YESNODIALOG_H
#define YESNODIALOG_H

#include "SimpleDialog.h"
#include "BaseDialog.h"
#include "Vector_ss.h"

namespace com { namespace glu { namespace platform { namespace graphics {
class CFont;
} } } }

/** Dialog with a text and the items "yes" (id 1) and "no" (id 2); closes on a click. */
class YesNoDialog : public SimpleDialog {
public:
    /** Item ids. */
    enum {
        ITEM_YES = 1,
        ITEM_NO = 2
    };

    /** `font` NULL: the item font. The three values are stored for the users of the dialog. */
    YesNoDialog(const XString& text, int userValue1, int userValue2, int userValue3,
                com::glu::platform::graphics::CFont* font);

    virtual ~YesNoDialog()
    {
    }

    virtual void Init();
    virtual void OnClosed();
    virtual void OnLostFocus();
    virtual void OnLayout();
    virtual void ClickItem(int id, bool byPointer);
    virtual void InitCenter();

    int m_userValue1;       // 0x178 not read in this file
    int m_userValue2;       // 0x17c
    int m_userValue3;       // 0x180
    int m_clickedItem;      // 0x184 id of the clicked item, 0 until then
    bool m_field188;        // 0x188 cleared, not read in this file
};

/** First start question: play with sound? Taps on the screen edges enter a cheat code. */
class GameStartSoundDialog : public YesNoDialog {
public:
    /** Records taps on the left / right third of the screen and compares them with the cheat code. */
    class CheatsWindow : public WindowTransparent {
    public:
        CheatsWindow();

        inline virtual ~CheatsWindow()
        {
        }

        virtual void OnPointerPressed(Event& event);

        /** 1 when the last `length` taps equal the start of the cheat code. */
        int CheckCheatTable(int length);

        /** Images of m_images. */
        enum {
            IMAGE_PARTIAL = 0,      // 4..6 taps of the code entered
            IMAGE_ALMOST = 1,       // 7..8 taps
            IMAGE_ENABLED = 2,      // cheats enabled
            IMAGE_COUNT = 3
        };

        /** Taps of the code: 1 left third, 0 right third. */
        static const signed char cheatTable[12];

        ImageRes m_images[IMAGE_COUNT];     // 0xac
        ImageWindow* m_image;               // 0xc4 shows the progress of the code
        bool m_cheatsEnabled;               // 0xc8
        Vector<signed char> m_taps;         // 0xcc the last taps
    };

    GameStartSoundDialog();

    virtual ~GameStartSoundDialog()
    {
    }

    virtual void Init();
    virtual void Shutdown();
    virtual void ClickItem(int id, bool byPointer);
    virtual void CancelDialog();
};

/** A message with a single "OK" item. */
class SimpleMessage : public YesNoDialog {
public:
    SimpleMessage(const XString& text, int userValue);

    virtual ~SimpleMessage()
    {
    }
};

#endif
