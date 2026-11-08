// CDH_SoftkeysScreen.cpp - base of the game screens that own a soft key bar.
#include "CDH_SoftkeysScreen.h"

CDH_SoftkeysScreen::CDH_SoftkeysScreen()
    : Window(false)
{
}

CDH_SoftkeysScreen::~CDH_SoftkeysScreen()
{
}

Window* CDH_SoftkeysScreen::PopulateSofkeys()
{
    return NULL;
}

void CDH_SoftkeysScreen::Init()
{
    // The soft key bar is a sibling of the screen, added in front of it.
    Window* softkeys = PopulateSofkeys();
    if (softkeys != NULL) {
        m_parent->AddToFront(softkeys);
    }
}
