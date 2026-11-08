// PROVISIONAL (created by b057x): owner src/bounty_hunter/Manager/CSceneObjectManager.cpp not written yet
// CSceneObjectManager.h - scene objects; only what src/general/App.cpp uses is declared.
// b078: complete manager and record layout from this unit; historical notes retained.
#ifndef CSCENEOBJECTMANAGER_H
#define CSCENEOBJECTMANAGER_H

#include <stddef.h>
#include "xString.h"  // added by b078
#include "Vector_ss.h"
#include "tws.h"
class TiXmlNode;
class CSceneObject;

class CSceneObjectManager {
public:
    /** Non-polymorphic config record; the model handle owns a runtime reference. */
    struct SSceneObjectInfo {  // added by b078
        int m_type;                   // 0x00 explosive 0 / spawn 1
        XString m_name;                // 0x04
        const char* m_modelResource;   // 0x08
        XString m_objectTransform;     // 0x0c
        XString m_meshTransform;       // 0x10
        XString m_lightNodeName;       // 0x14
        mutable swerve::Group<CSwerveAssertingErrorHandler> m_model;  // 0x18, model lazily cached by the original SceneObject const config
    };
    struct SSpawnSceneObjectInfo : SSceneObjectInfo {
        SSpawnSceneObjectInfo() { m_type = 1; }
    };
    struct SExplosiveSceneObjectInfo : SSceneObjectInfo {
        SExplosiveSceneObjectInfo() { m_type = 0; }
        int m_damage;           // 0x1c
        float m_damageRadius;   // 0x20
        int m_hp;               // 0x24
    };
    CSceneObjectManager();
    ~CSceneObjectManager();
    SSceneObjectInfo* GetSceneObjectInfoByName(const XString& name);  // added by b078
    void Parse(TiXmlNode* node);
    void ParseExplosiveObject(TiXmlNode* node);
    void ParseObject(TiXmlNode* node, SSceneObjectInfo* info);
    void CleanupResources();
    CSceneObject* CreateSceneObject(const SSceneObjectInfo* info);

    // Former m_data00: layout not known yet (size from App.cpp).
    Vector<SSceneObjectInfo*> m_objects;  // 0x00
};

#endif
