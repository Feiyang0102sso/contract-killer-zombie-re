// CAchievementManager.h - achievements of res/common/configs/achievements.xml, their progress
// (saved with the profile) and posting to Game Center.
#ifndef CACHIEVEMENTMANAGER_H
#define CACHIEVEMENTMANAGER_H

#include <stddef.h>
#include "xString.h"
#include "Vector_ss.h"
#include "HashTable.h"

class TiXmlNode;
class TiXmlElement;

/** One achievement: reached in m_steps steps of m_value / m_steps each. */
class CAchievement {
public:
    CAchievement()
    {
        m_progress = 0;
        m_steps = 1;
        m_synced = true;
    }

    XString m_name;                     // 0x00 Game Center identifier
    int m_currentValue;                 // 0x04 last value checked
    int m_value;                        // 0x08 value of the last step
    int m_progress;                     // 0x0c steps reached
    int m_steps;                        // 0x10
    XString m_imageId;                  // 0x14
    XString m_screenName;               // 0x18
    XString m_description;              // 0x1c
    XString m_completedDescription;     // 0x20
    bool m_hidden;                      // 0x24
    bool m_synced;                      // 0x25 progress posted to Game Center
    int m_points;                       // 0x28
};

class CAchievementManager {
public:
    CAchievementManager();
    ~CAchievementManager();

    void ParseAchievements(TiXmlNode* node);
    /** Write the progress under <achievements>. */
    void Save(TiXmlElement* root);
    void Load(TiXmlElement* root);
    /** Check every achievement against the player statistics (`checkScores`: also the title / score ones). */
    void CheckAllAchievements(bool checkScores);
    /** Give the steps of `name` reached by `value` and post the progress to Game Center. */
    void CheckAchievement(XString name, int value);
    /** Set the progress of `name` to `progress` steps; true when it changed. */
    bool GiveAchievement(XString name, int progress);
    /** Check the achievements now and then (`force`: a longer interval, no Game Center fetch). */
    void Update(bool force);
    /** Sum of the points of all achievements (0 when one is missing). */
    int GetOverallAchievementScore();
    void CheckScoreAchievements();

    unsigned int m_lastCheckTime;                       // 0x00 uptime in ms
    HashTable<XString, CAchievement*> m_achievements;   // 0x04
    Vector<XString> m_names;                            // 0x10 in file order
};

#endif
