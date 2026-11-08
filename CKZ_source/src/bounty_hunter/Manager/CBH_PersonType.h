// CBH_PersonType.h - one kind of person (enemy, civilian, boss) read from persons.xml.
#ifndef CBH_PERSONTYPE_H
#define CBH_PERSONTYPE_H

#include "xString.h"
#include "Vector_ss.h"

class TiXmlNode;
class CEnemyWeaponType;
class CVisualCharacterType;

class CBH_PersonType {
public:
    CBH_PersonType(TiXmlNode* node);

    /** Read the attributes of a <person> node. */
    void Parse(TiXmlNode* node);
    /** Visual character picked with the global seed (`index` is not used). */
    CVisualCharacterType* GetVisualObject(unsigned int index) const;
    /** Random damage in [min, max]; multiplied on a critical hit. */
    int GetAttackDamage() const;
    /** Random scale in [m_scaleMin, m_scaleMax]. */
    float GenerateRandScale() const;
    /** New seed for GetVisualObject. */
    static void GenerateGlobalSeed();

    static unsigned int m_globalSeed;

    XString m_name;                         // 0x00
    XString m_scriptName;                   // 0x04
    Vector<XString> m_visualCharacters;     // 0x08
    CEnemyWeaponType* m_weapon;             // 0x18
    int m_baseHp;                           // 0x1c
    int m_damageMin;                        // 0x20
    int m_damageMax;                        // 0x24
    int m_blowDamage;                       // 0x28
    float m_critProbability;                // 0x2c
    float m_critMultiplier;                 // 0x30
    int m_damageCooldown;                   // 0x34
    float m_attackRange;                    // 0x38
    float m_fearRange;                      // 0x3c
    float m_dangerRange;                    // 0x40
    float m_specialAttackRange;             // 0x44
    float m_arcadeScore;                    // 0x48
    float m_arcadeCash;                     // 0x4c
    float m_arcadeXp;                       // 0x50
    bool m_ignoreLureGrenades;              // 0x54
    bool m_isCivilian;                      // 0x55
    bool m_showHudInfo;                     // 0x56
    float m_scaleMin;                       // 0x58
    float m_scaleMax;                       // 0x5c
    bool m_isBoss;                          // 0x60
};

#endif
