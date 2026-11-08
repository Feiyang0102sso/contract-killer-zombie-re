// LanguageDialog.h - first start dialog that lets the player choose the game language.
#ifndef LANGUAGEDIALOG_H
#define LANGUAGEDIALOG_H

#include "SimpleDialog.h"

class SelectLanguageDialog : public SimpleDialog {
public:
    SelectLanguageDialog();

    virtual ~SelectLanguageDialog()
    {
    }

    virtual void OnStartupEasy();
    virtual void Init();
    virtual void Shutdown();
    virtual ItemFonts ItemFontsByIndex(int index) const;
    virtual void ClickItem(int index, bool byPointer);
    virtual void AdjustByDefRect();
    virtual void InitCenter();

    com::glu::platform::graphics::CFont* m_itemFont;            // 0x178
    com::glu::platform::graphics::CFont* m_selectedItemFont;    // 0x17c
    int m_selectedLanguage;                                     // 0x180 -1: none chosen
};

#endif
