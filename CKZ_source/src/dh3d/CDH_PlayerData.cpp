// CDH_PlayerData.cpp - saved progress of the player.
#include "CDH_PlayerData.h"
#include "CDH_QuickGameData.h"
#include "CDH_GameDayData.h"
#include "BetaPlayerData.h"

CDH_PlayerData::CDH_PlayerData()
{
    m_gameDayData = new CDH_GameDayData();
    m_quickGameData = new CDH_QuickGameData();
    m_quickGameData2 = new CDH_QuickGameData();
    m_betaData = new BetaPlayerData();
    reset();
}

void CDH_PlayerData::reset()
{
    m_field60[0] = 0;
    m_field60[1] = 0;
    m_field60[2] = 0;
    m_field60[3] = 0;
    m_field60[4] = 0;
    m_field60[5] = 0;
    m_field60[6] = 0;
    m_field2c = 0;
    for (int index = 0; index < SKILL_COUNT; index++) {
        m_skillLevels[index] = 0;
    }
    m_achievements.Reset();
    m_field88 = 1;
    m_field84 = 0;
    m_field90 = 0;
    m_field94 = 200000;
    m_field98 = 10000;
    m_field9c = 15;
    m_field8c = 0;
    m_fielda4 = false;
    m_fielda8 = 1.0f;
    m_fieldac = 0;
    m_betaData->reset();
}

CDH_PlayerData::~CDH_PlayerData()
{
    if (m_quickGameData != NULL) {
        delete m_quickGameData;
        m_quickGameData = NULL;
    }
    if (m_quickGameData2 != NULL) {
        delete m_quickGameData2;
        m_quickGameData2 = NULL;
    }
    if (m_gameDayData != NULL) {
        delete m_gameDayData;
        m_gameDayData = NULL;
    }
    if (m_betaData != NULL) {
        delete m_betaData;
        m_betaData = NULL;
    }
}

float CDH_PlayerData::GetSkillLevelCoef(skill_type_t skill) const
{
    return (char)m_skillLevels[skill] / 5.0f;
}

CDH_BasicGameData* CDH_PlayerData::GetGameData(game_type_t type)
{
    return (CDH_BasicGameData*)m_quickGameData;
}

void CDH_PlayerData::Serialize(DataOutputStream& stream)
{
    for (int index = 0; index < SKILL_COUNT; index++) {
        stream.writeInt(m_skillLevels[index]);
    }
    m_achievements.Serialize(stream);
    stream.writeByte(m_field2c);
    ((Serializable*)m_gameDayData)->Serialize(stream);
    stream.writeInt(m_field84);
    stream.writeInt(m_field88);
    stream.writeInt(m_field94);
    stream.writeInt(m_field98);
    stream.writeInt(m_field9c);
    stream.writeInt(m_field8c);
    stream.writeByte(m_fielda4);
    m_betaData->Serialize(stream);
}

void CDH_PlayerData::DeSerialize(JDataInputStream& stream)
{
    for (int index = 0; index < SKILL_COUNT; index++) {
        m_skillLevels[index] = stream.readInt();
    }
    m_achievements.DeSerialize(stream);
    m_field2c = stream.readByte();

    if (m_quickGameData != NULL) {
        delete m_quickGameData;
        m_quickGameData = NULL;
    }
    if (m_quickGameData2 != NULL) {
        delete m_quickGameData2;
        m_quickGameData2 = NULL;
    }
    m_quickGameData = new CDH_QuickGameData();
    m_quickGameData2 = new CDH_QuickGameData();

    if (m_gameDayData != NULL) {
        delete m_gameDayData;
        m_gameDayData = NULL;
    }
    m_gameDayData = new CDH_GameDayData();
    m_gameDayData->DeSerialize(stream);

    m_field7c = 0;
    m_field84 = stream.readInt();
    m_field88 = stream.readInt();
    m_field94 = stream.readInt();
    m_field98 = stream.readInt();
    m_field9c = stream.readInt();
    m_field8c = stream.readInt();
    m_fielda4 = stream.readByte() != 0;

    if (m_betaData != NULL) {
        delete m_betaData;
        m_betaData = NULL;
    }
    m_betaData = new BetaPlayerData();
    m_betaData->DeSerialize(stream);
}
