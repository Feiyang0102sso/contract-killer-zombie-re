// CAchievements.h - achievements reached by the player (two lists of achievement ids).
#ifndef CACHIEVEMENTS_H
#define CACHIEVEMENTS_H

#include "Serializable.h"

class CAchievements : public Serializable {
public:
    /** Achievement id (values not known). */
    enum achievements_type_t {
        ACHIEVEMENT_NONE = 0
    };

    CAchievements();
    // Explicit (non-virtual) destructor: the original keeps it as a weak out-of-line D2
    // called from ~CDH_PlayerData; the implicit one got inlined (b117-inl).
    ~CAchievements() {}

    virtual void Serialize(DataOutputStream& stream);
    virtual void DeSerialize(JDataInputStream& stream);

    /** Forget every achievement. */
    void Reset();

    Vector<achievements_type_t> m_achievements;     // 0x04
    Vector<achievements_type_t> m_pending;          // 0x14 reached but not reported yet (guess)
};

#endif
