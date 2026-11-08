// CDH_QuickGameData.cpp - progress data of the quick game mode.
#include "CDH_QuickGameData.h"

CDH_QuickGameData::CDH_QuickGameData()
{
    m_field1c = 0;
}

CDH_QuickGameData::~CDH_QuickGameData()
{
}

void CDH_QuickGameData::Serialize(DataOutputStream& stream)
{
    CDH_BasicGameData::Serialize(stream);
    stream.writeByte(m_field1c);
}

void CDH_QuickGameData::DeSerialize(JDataInputStream& stream)
{
    CDH_BasicGameData::DeSerialize(stream);
    m_field1c = stream.readByte();
}

void CDH_QuickGameData::ResetImpl()
{
    m_field1c = 0;
}

void CDH_QuickGameData::FinishGameImpl()
{
}

void CDH_QuickGameData::StartGameImpl()
{
    ResetImpl();
}
