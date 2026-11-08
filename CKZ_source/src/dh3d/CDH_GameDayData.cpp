// CDH_GameDayData.cpp - progress data of the day based game mode.
#include "CDH_GameDayData.h"

CDH_GameDayData::CDH_GameDayData()
    : m_field2c(0),
      m_field30(true)
{
}

CDH_GameDayData::~CDH_GameDayData()
{
}

void CDH_GameDayData::Serialize(DataOutputStream& stream)
{
    CDH_BasicGameData::Serialize(stream);
    stream.writeByte(m_field30);
    SERIALIZE_VECTOR(m_achievements, stream);
    if (!m_hasData) {
        return;
    }
    int size = m_dayState.m_buffer.size();
    stream.writeInt(size);
    stream.write(m_dayState.GetData(), size);
}

void CDH_GameDayData::DeSerialize(JDataInputStream& stream)
{
    CDH_BasicGameData::DeSerialize(stream);
    m_field30 = stream.readByte() != 0;
    DESERIALIZE_VECTOR(m_achievements, stream);
    if (m_hasData) {
        m_dayState.clear();
        int size = stream.readInt();
        for (int index = 0; index < size; index++) {
            m_dayState.writeByte(stream.readByte());
        }
    }
}

void CDH_GameDayData::ResetImpl()
{
    m_dayState.clear();
    m_field2c = 0;
    m_field30 = true;
    m_achievements.removeAllElements();
}

void CDH_GameDayData::FinishGameImpl()
{
}

void CDH_GameDayData::StartGameImpl()
{
    ResetImpl();
}
