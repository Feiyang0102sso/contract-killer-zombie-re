// b074: original fields and added virtual slots, full size 0x150.
#ifndef CBH_PROGRESSBAR_H
#define CBH_PROGRESSBAR_H
#include "CBH_Button.h"
enum TextAlign { TEXT_LEFT = 0, TEXT_OUTSIDE = 1, TEXT_CENTER = 2 };
class CBH_ProgressBar : public CBH_Button {
public:
    CBH_ProgressBar(int color, int width, int secondColor);
    virtual ~CBH_ProgressBar();
    virtual void Init();
    virtual void Update();
    virtual void Paint(com::glu::platform::graphics::ICGraphics2d& graphics);
    virtual void SetCurrValue(int value); // c0
    virtual void ReleaseResources();      // c4
    int GetFillWidth();
    void CreateImpl(int width);
    void SetTextAlign(TextAlign align);
    void SetDescImage(const char* name);
    void UpdateBarParams();
    float m_delta;                 // e4
    int m_maximum;                 // e8
    int m_value;                   // ec
    float m_percent;               // f0
    int m_unresolvedF4[2];         // f4/f8, original constructor does not initialize
    const char* m_descName;        // fc borrowed
    int m_unresolved100;           // 100
    unsigned int m_barFlags;       // 104
    TextAlign m_textAlign;         // 108
    int m_fontAlign;               // 10c
    int m_textX;                   // 110
    int m_textY;                   // 114
    XString m_text;                // 118
    const char* m_leftName;        // 11c
    const char* m_rightName;       // 120
    const char* m_fillName;        // 124
    const char* m_colorName;       // 128
    const char* m_secondColorName; // 12c
    com::glu::platform::graphics::ICRenderSurface* m_color;       // 130
    com::glu::platform::graphics::ICRenderSurface* m_secondColor; // 134
    com::glu::platform::graphics::ICRenderSurface* m_desc;        // 138
    com::glu::platform::graphics::ICRenderSurface* m_left;        // 13c
    com::glu::platform::graphics::ICRenderSurface* m_right;       // 140
    com::glu::platform::graphics::ICRenderSurface* m_fill;        // 144
    int m_displayFormat;           // 148
    bool m_showText;               // 14c
};
#endif
