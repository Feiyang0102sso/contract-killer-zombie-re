// CAuxRenderText.cpp - debug text overlay; the text functions are empty in the release build.
#include "CAuxRenderText.h"

CAuxRenderText::CAuxRenderText()
    : CSingleton(SingletonId)
{
    m_textCount = 0;
}

void CAuxRenderText::AddTextInfo(const wchar_t* format, ...)
{
}

void CAuxRenderText::AddTextInfoXY(int x, int y, const wchar_t* format, ...)
{
}
