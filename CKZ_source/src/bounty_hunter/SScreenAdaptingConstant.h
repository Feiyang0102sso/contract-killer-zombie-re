// PROVISIONAL (created by b079): owner src/bounty_hunter/CBH_Button.cpp not written yet
// Four-int argument layout from CTutorialFloatingWindow::SetText; operator @0x1154d0.
#ifndef SSCREENADAPTINGCONSTANT_H
#define SSCREENADAPTINGCONSTANT_H
#include "App.h"

struct SScreenAdaptingConstant {
    SScreenAdaptingConstant(int value, int second = -1, int third = -1, int fourth = -1)
    {
        m_values[0] = value;
        m_values[1] = second;
        m_values[2] = third;
        m_values[3] = fourth;
    }

    // b074: original weak operator 0x1154d0; retain repeated platform queries.
    int operator()() const
    {
        if (m_values[2] != -1 && App::IsXGA() == 1) {
            return m_values[2];
        }
        if (m_values[3] != -1 && App::IsWVGA() == 1) {
            return m_values[3];
        }
        if (m_values[1] != -1) {
            if (App::IsWVGA() == 1) {
                return (m_values[1] * 8) / 10;
            }
            if (m_values[1] != -1 && App::IsXGA() != 0) {
                return m_values[1];
            }
            if (m_values[1] != -1 && App::IsVGA() == 1) {
                return m_values[1];
            }
        }
        if (m_values[2] != -1 && App::IsVGA() == 1) {
            return m_values[2];
        }
        if (App::IsHD() == 1) {
            if (App::IsWVGA() == 1) {
                return (int)((double)m_values[0] * 1.6);
            }
            return (int)(double)(m_values[0] << 1);
        }
        return m_values[0];
    }
    int m_values[4];
};

#endif
