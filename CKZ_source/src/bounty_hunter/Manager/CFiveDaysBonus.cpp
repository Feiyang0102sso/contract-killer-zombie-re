// CFiveDaysBonus.cpp - the daily login bonus (five_days_bonus.xml).
#include "CFiveDaysBonus.h"
#include <wchar.h>
#include "CApplet.h"
#include "CApp.h"
#include "CHash.h"
#include "CRandGen.h"
#include "App.h"
#include "ResMap.h"
#include "tinyxml.h"
#include "CXmlHelper.h"
#include "CShopItem.h"
#include "CDH_WeaponsManager.h"
#include "CBH_Player.h"
#include "CGameAnalytics.h"

using namespace com::glu::platform::core;

CFiveDaysBonus::CFiveDaysBonus()
    : m_day(-1), m_chance(-1)
{
    ResMap* resMap = ((App*)WindowApp::m_instance)->m_resMap;
    char* text = resMap->loadText(XString(L"res/common/configs/five_days_bonus.xml"));
    TiXmlDocument document;
    document.Parse(text);
    if (text != NULL) {
        delete[] text;
    }
    TiXmlNode* root = document.FirstChild("fiveDaysBonus");
    ParseFourDay(root);
    ParseFifthDay(root);
}

void CFiveDaysBonus::ParseFourDay(TiXmlNode* node)
{
    if (node == NULL) {
        return;
    }
    TiXmlNode* child = node->FirstChild("day");
    for (unsigned int day = 0; child != NULL && day < 5; day++) {
        m_days[day].m_cash = CXmlHelper::GetAttributeValueOrUseDefault(child, "cash", XString(L"0")).ToInt();
        m_days[day].m_credits = CXmlHelper::GetAttributeValueOrUseDefault(child, "credits", XString(L"0")).ToInt();
        child = child->NextSibling("day");
    }
}

void CFiveDaysBonus::ParseFifthDay(TiXmlNode* node)
{
    if (node == NULL) {
        return;
    }
    m_fifthDays.ensureCapacity(5);
    for (TiXmlNode* child = node->FirstChild("fifth_day"); child != NULL; child = child->NextSibling("fifth_day")) {
        SFifthDay prize;
        prize.m_probability = CXmlHelper::GetAttributeValue(child, "probability").ToInt();
        prize.m_cash = CXmlHelper::GetAttributeValue(child, "cash").ToInt();
        prize.m_credits = CXmlHelper::GetAttributeValue(child, "credits").ToInt();
        prize.m_medkits = CXmlHelper::GetAttributeValue(child, "medkits").ToInt();
        prize.m_weaponId = CXmlHelper::GetAttributeValue(child, "weaponID").ToInt();
        m_fifthDays.addElement(prize);
    }
}

void CFiveDaysBonus::GiveBonus(int day)
{
    if ((unsigned int)day > 5) {
        return;
    }
    m_day = day;
    if (day < 5) {
        if (m_days[day].m_cash > 0) {
            CBH_Player::GetInstance()->AddMoney(m_days[m_day].m_cash);
            XString bonus = XString::Format(L"%d cash", m_days[m_day].m_cash);
            CGameAnalytics::logDailyBonus(bonus, m_day);
        } else if (m_days[day].m_credits > 0) {
            CBH_Player::GetInstance()->AddCredits(m_days[m_day].m_credits, L"");
            XString bonus = XString::Format(L"%d credits", m_days[m_day].m_credits);
            CGameAnalytics::logDailyBonus(bonus, m_day);
        }
        return;
    }
    // the last day: roll one of the prizes by their probabilities
    int roll = CRandGen::GetInstance()->GetRandRange(0, 100);
    int sum = 0;
    for (int index = 0; index < m_fifthDays.size(); index++) {
        sum += m_fifthDays[index].m_probability;
        if (roll < sum) {
            GiveChance(index);
            return;
        }
    }
}

void CFiveDaysBonus::GiveChance(int index)
{
    m_chance = index;
    SFifthDay& prize = m_fifthDays[index];
    // b147: original lays out the weapon branch first (cmp #0; blt to the cash/credits branch).
    if (prize.m_weaponId >= 0) {
        CItem* weapon = ((App*)WindowApp::m_instance)->m_weaponsManager->getItemById(prize.m_weaponId);
        if (weapon == NULL) {
            return;
        }
        if (weapon->isAvailable() == true) {
            // already owned: the cash of the prize instead
            int cash = m_fifthDays[index].m_cash;
            m_chanceText = XString::Format(Window::ResString("IDS_FIVE_DAYS_BONUS_MONEY").c_str(), cash);
            CBH_Player::GetInstance()->AddMoney(cash);
            XString bonus = XString::Format(L"%d cash", cash);
            CGameAnalytics::logDailyBonus(bonus, m_day);
        } else {
            weapon->setAvailable(true);
            m_chanceText = weapon->GetName();
            XString bonus = XString::Format(L"%s unlocked", weapon->GetName().c_str());
            CGameAnalytics::logDailyBonus(bonus, m_day);
        }
    } else {
        if (prize.m_cash > 0) {
            CBH_Player::GetInstance()->AddMoney(m_fifthDays[index].m_cash);
            m_chanceText = XString::Format(Window::ResString("IDS_FIVE_DAYS_BONUS_MONEY").c_str(), m_fifthDays[index].m_cash);
            XString bonus = XString::Format(L"%d cash", m_fifthDays[index].m_cash);
            CGameAnalytics::logDailyBonus(bonus, m_day);
        } else if (prize.m_credits > 0) {
            CBH_Player::GetInstance()->AddCredits(m_fifthDays[index].m_credits, L"Daily Bonus");
            m_chanceText = XString::Format(Window::ResString("IDS_FIVE_DAYS_BONUS_CREDITS").c_str(), m_fifthDays[index].m_credits);
            XString bonus = XString::Format(L"%d credits", m_fifthDays[index].m_credits);
            CGameAnalytics::logDailyBonus(bonus, m_day);
        }
    }
}

int CFiveDaysBonus::GetDay() const
{
    return m_day;
}

const char* CFiveDaysBonus::GetDayBonusIcon(int day) const
{
    if (day < 5) {
        if (m_days[day].m_cash > 0) {
            return "IDB_SOFT_1_PNG";
        }
        return "IDB_HARD_1_PNG";
    }
    return "IDB_SOFT_1_PNG";
}

XString CFiveDaysBonus::GetDayBonusNameForIcon(int day) const
{
    if (day < 5) {
        if (m_days[day].m_cash < 1) {
            return XString::Format(Window::ResString("IDS_FIVE_DAYS_BONUS_CREDITS").c_str(), m_days[day].m_credits);
        }
        return XString::Format(Window::ResString("IDS_FIVE_DAYS_BONUS_MONEY").c_str(), m_days[day].m_cash);
    }
    return Window::ResString("IDS_FIVE_DAYS_BONUS_CHANCE");
}

XString CFiveDaysBonus::GetDayBonusNameForMessage(int day) const
{
    if (day < 5) {
        if (m_days[day].m_cash < 1) {
            return XString::Format(Window::ResString("IDS_FIVE_DAYS_BONUS_CREDITS").c_str(), m_days[day].m_credits);
        }
        return XString::Format(Window::ResString("IDS_FIVE_DAYS_BONUS_MONEY").c_str(), m_days[day].m_cash);
    }
    return m_chanceText;
}
