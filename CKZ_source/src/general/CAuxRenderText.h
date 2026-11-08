// CAuxRenderText.h - debug text overlay (singleton); the text functions are compiled out
// in the release build.
#ifndef CAUXRENDERTEXT_H
#define CAUXRENDERTEXT_H

#include "CSingleton.h"

class CAuxRenderText : public com::glu::platform::components::CSingleton {
public:
    enum { SingletonId = 0x88510fb7 };

    /** The overlay of the running application, created on first use. */
    static inline CAuxRenderText* GetInstance()
    {
        void* found = NULL;
        CApplet::m_pApp->m_singletons->Find(SingletonId, &found);
        CAuxRenderText* instance = (CAuxRenderText*)found;
        if (instance == NULL) {
            instance = new CAuxRenderText();
        }
        return instance;
    }

    CAuxRenderText();

    virtual ~CAuxRenderText()
    {
    }

    /** Queue a debug line (no-op in release). */
    void AddTextInfo(const wchar_t* format, ...);
    /** Queue a debug text at a screen position (no-op in release). */
    void AddTextInfoXY(int x, int y, const wchar_t* format, ...);

    int m_textCount;    // 0x08 queued texts (only cleared in this build)
};

#endif
