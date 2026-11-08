// Original u468 complete owner; the third Update triggers the interrupt, no extra Window::Update call is added.
#include "CIntermediateState.h"

CIntermediateState::CIntermediateState() : Window(false), m_updateCount(0) {}

void CIntermediateState::Update()
{
    if (m_updateCount == 2) {
        SetInterrupt(0);
    }
    ++m_updateCount; // The virtual callback can modify this field; keep the original reread after the callback.
}
