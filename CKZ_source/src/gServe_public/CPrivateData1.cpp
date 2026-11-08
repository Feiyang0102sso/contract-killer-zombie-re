// CPrivateData1.cpp - getters of the game's private profile collection.
#include "CPrivateData1.h"

long long CPrivateData1::getSoftCurrency()
{
    return m_softCurrency;
}

long long CPrivateData1::getHardCurrency()
{
    return m_hardCurrency;
}

long long CPrivateData1::getExperience()
{
    return m_experience;
}

unsigned char CPrivateData1::getSoundSetting()
{
    return m_soundSetting;
}

unsigned char CPrivateData1::getMusicSetting()
{
    return m_musicSetting;
}

unsigned char CPrivateData1::getFireBtnSetting()
{
    return m_fireBtnSetting;
}

unsigned char CPrivateData1::getNotificationSetting()
{
    return m_notificationSetting;
}
