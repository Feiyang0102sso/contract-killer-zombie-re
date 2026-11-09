// CVisualCharacterType.h - character models, LODs, pooled copies, animations and event sounds.
#ifndef CVISUALCHARACTERTYPE_H
#define CVISUALCHARACTERTYPE_H

#include "CEnemyWeaponType.h"
#include "CStrChar.h"
#include "SwerveHelper.h"

class TiXmlNode;
class AnimationImpl;

class CVisualCharacterType {
public:
    typedef swerve::Object3D<CSwerveAssertingErrorHandler> Object3D;
    typedef swerve::Group<CSwerveAssertingErrorHandler> Group;
    typedef com::glu::platform::components::CStrChar CStrChar;

    /** Named animation resources and the choices for each of the 37 person animation types. */
    class SharedAnimation {
    public:
        SharedAnimation();
        ~SharedAnimation();
        XString m_name;                         // 0x000
        Vector<AnimationImpl*> m_animations;    // 0x004
        Vector<int> m_tracks[37];               // 0x014
        const char* m_mapResourceID;            // 0x264
        const char* m_anmResourceID;            // 0x268
    };

    CVisualCharacterType(TiXmlNode* node);
    ~CVisualCharacterType();
    void Parse(TiXmlNode* node);
    void CleanupResources() const;
    static void LoadCommonInfo(TiXmlNode* node);
    static void LoadSharedInfo(TiXmlNode* node);
    static void ParseAnimationTracks(TiXmlNode* node, SharedAnimation* animation);
    static void LoadCommonAnimation(TiXmlNode* node);
    static void DestroyCommonAnimation();
    Group GetCharacterModelCopy() const;
    void LoadModel() const;
    void ReleaseCharacterModelCopy(Group group) const;
    const char* GetLodName(unsigned int lod) const;
    float GetLodDistance(unsigned int lod) const;
    const char* GetLodHeadGroupName(unsigned int lod) const;
    AnimationImpl* GetAnimation(EPersonAnimType type) const;
    void ParseInfo(TiXmlNode* node);
    void ParseSound(TiXmlNode* node);
    void ParseModel(TiXmlNode* node);
    void ParseLods(TiXmlNode* node);
    void ReplaceTexture(Object3D body) const;
    void ReplaceTexture(Object3D body, Object3D head) const;
    void LoadSounds() const;
    void PreloadEventSound(XString sound) const;
    static int GetHitzoneMeshUserId();
    static int GetHitzoneGroupUserId();
    int PlayEventSound(XString sound, float delay) const;
    int PlayAttackSound() const;
    int PlayWoundSound() const;
    int PlayDeathSound() const;
    /** Original Body0x107fa4: distance interpolated to the far quantum; NaN is handled by the final ARM float-to-int conversion. */
    int GetAnimLodQuantumMS(float distance, int nearQuantumMs) const
    {
        float factor = (distance - m_nearDistance) / (m_farDistance - m_nearDistance);
        if (factor > 1.0f) {
            factor = 1.0f;
        } else {
            factor = (factor < 0.0f) ? 0.0f : factor;
        }
        return (int)((float)nearQuantumMs * (1.0f - factor) + (float)m_farQuantumMS * factor);
    }

    static const char* m_pBodyDummyName;
    static float m_defaultWalkSpeedMan;
    static float m_defaultWalkSpeedWoman;
    static float m_defaultRunSpeedMan;
    static float m_defaultRunSpeedWoman;
    static float m_defaultRunWoundSpeed;
    static float m_defaultCarrySpeed;
    static Vector<AnimationImpl*> m_commonAnimation;
    static const char* m_commonMapResourceID;
    static const char* m_commonAnmResourceID;
    static Vector<SharedAnimation*> m_sharedAnimations;

    float m_walkSpeed;                         // 0x000 initialized only by ParseInfo
    float m_runSpeed;                          // 0x004
    float m_runWoundSpeed;                     // 0x008
    float m_carrySpeed;                        // 0x00c
    mutable bool m_loaded;                    // 0x010
    mutable Object3D m_model;                 // 0x014
    int m_field18;                            // 0x018
    mutable Vector<Group> m_modelCopies;      // 0x01c
    mutable Bounds3 m_bounds;                 // 0x02c
    XString m_name;                           // 0x044
    XString m_exclamationSound;               // 0x048
    XString m_attackSound;                    // 0x04c
    XString m_woundSound;                     // 0x050
    XString m_deathSound;                     // 0x054
    XString m_bodyResource;                   // 0x058
    XString m_headResource;                   // 0x05c
    XString m_bodyTexture;                    // 0x060
    XString m_headTexture;                    // 0x064
    XString m_legsTexture;                    // 0x068
    int m_field6c;                            // 0x06c initialized only by ParseInfo
    CStrChar m_rootDummy;                     // 0x070
    CStrChar m_bodyTextureNode;               // 0x080
    CStrChar m_legsTextureNode;               // 0x090
    CStrChar m_headTextureNode;               // 0x0a0
    CStrChar m_lightTextureNode;              // 0x0b0
    bool m_effect;                           // 0x0c0 initialized only by ParseModel
    bool m_hasLodConfig;                      // 0x0c1
    float m_nearDistance;                     // 0x0c4
    float m_farDistance;                      // 0x0c8
    int m_farQuantumMS;                       // 0x0cc
    int m_quality;                            // 0x0d0
    unsigned int m_lodCount;                  // 0x0d4
    CStrChar m_lodNames[3];                   // 0x0d8
    CStrChar m_headGroupBody[3];              // 0x108
    CStrChar m_headGroupHead[3];              // 0x138
    CStrChar m_specialHitZoneNodes[3];        // 0x168
    int m_specialHitZoneIds[3];              // 0x198 set only when the attribute exists
    float m_lodDistances[3];                 // 0x1a4
    SharedAnimation* m_sharedAnimation;       // 0x1b0
    bool m_isWoman;                          // 0x1b4
    bool m_isMan;                            // 0x1b5
    bool m_noFrustumCullingOnSpawn;           // 0x1b6
    Vector<XString> m_pickingDisabledNodes;  // 0x1b8
};

#endif
