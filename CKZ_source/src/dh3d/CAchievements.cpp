// CAchievements.cpp - achievements reached by the player.
#include "CAchievements.h"

CAchievements::CAchievements()
{
}

void CAchievements::Serialize(DataOutputStream& stream)
{
    SERIALIZE_VECTOR(m_achievements, stream);
    SERIALIZE_VECTOR(m_pending, stream);
}

void CAchievements::DeSerialize(JDataInputStream& stream)
{
    DESERIALIZE_VECTOR(m_achievements, stream);
    DESERIALIZE_VECTOR(m_pending, stream);
}

void CAchievements::Reset()
{
    m_achievements.removeAllElements();
    m_pending.removeAllElements();
}
