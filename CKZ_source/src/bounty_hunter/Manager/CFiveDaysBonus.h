// CFiveDaysBonus.h - the daily login bonus of res/common/configs/five_days_bonus.xml: cash or
// credits for the first five days, a random prize (cash, credits or a weapon) on the sixth.
#ifndef CFIVEDAYSBONUS_H
#define CFIVEDAYSBONUS_H

#include <stddef.h>
#include "Vector_ss.h"
#include "xString.h"

class TiXmlNode;

class CFiveDaysBonus {
public:
    /** Bonus of one of the first days. */
    struct SDayBonus {
        int m_cash;         // 0x00
        int m_credits;      // 0x04
    };

    /** One possible prize of the last day. */
    struct SFifthDay {
        int m_probability;  // 0x00 percent
        int m_cash;         // 0x04
        int m_credits;      // 0x08
        int m_medkits;      // 0x0c
        int m_weaponId;     // 0x10 -1: no weapon
    };

    CFiveDaysBonus();

    void ParseFourDay(TiXmlNode* node);
    void ParseFifthDay(TiXmlNode* node);
    /** Give the bonus of `day` (0..5; 5 rolls one of the prizes). */
    void GiveBonus(int day);
    /** Give the last-day prize `index`. */
    void GiveChance(int index);
    int GetDay() const;
    const char* GetDayBonusIcon(int day) const;
    XString GetDayBonusNameForIcon(int day) const;
    XString GetDayBonusNameForMessage(int day) const;

    SDayBonus m_days[5];                // 0x00
    Vector<SFifthDay> m_fifthDays;      // 0x28
    XString m_chanceText;               // 0x38 text of the last-day prize
    int m_day;                          // 0x3c -1: none given yet
    int m_chance;                       // 0x40 -1: none given yet
};

#endif
