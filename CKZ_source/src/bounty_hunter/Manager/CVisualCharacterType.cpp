// CVisualCharacterType.cpp - XML character definitions and their shared native resources.
#include "CVisualCharacterType.h"
#include "CVisualCharacterTypeManager.h"
#include "CXmlHelper.h"
#include "App.h"
#include "ResMap.h"
#include "DGHelper.h"
#include "CSwerveGame.h"
#include "AnimationImpl.h"
#include "CDH_SoundManager.h"
#include "CRandGen.h"
#include "AdBannerSizes.h"

using com::glu::platform::core::CRandGen;
typedef swerve::Node<CSwerveAssertingErrorHandler> VisualNode;
typedef swerve::Mesh<CSwerveAssertingErrorHandler> VisualMesh;
typedef swerve::World<CSwerveAssertingErrorHandler> VisualWorld;
typedef swerve::Image2D<CSwerveAssertingErrorHandler> VisualImage;
typedef swerve::Transform<CSwerveAssertingErrorHandler> VisualTransform;

const char* CVisualCharacterType::m_pBodyDummyName = "dummy_guyTransform";
float CVisualCharacterType::m_defaultWalkSpeedMan;
float CVisualCharacterType::m_defaultWalkSpeedWoman;
float CVisualCharacterType::m_defaultRunSpeedMan;
float CVisualCharacterType::m_defaultRunSpeedWoman;
float CVisualCharacterType::m_defaultRunWoundSpeed;
float CVisualCharacterType::m_defaultCarrySpeed;
Vector<AnimationImpl*> CVisualCharacterType::m_commonAnimation;
const char* CVisualCharacterType::m_commonMapResourceID;
const char* CVisualCharacterType::m_commonAnmResourceID;
Vector<CVisualCharacterType::SharedAnimation*> CVisualCharacterType::m_sharedAnimations;

CVisualCharacterType::SharedAnimation::SharedAnimation()
    : m_mapResourceID(NULL), m_anmResourceID(NULL)
{
}

CVisualCharacterType::SharedAnimation::~SharedAnimation()
{
    SAFE_DELETE_VECTOR(m_animations);
}

CVisualCharacterType::CVisualCharacterType(TiXmlNode* node)
    : m_loaded(false), m_field18(0), m_hasLodConfig(false), m_nearDistance(0.0f),
      m_farDistance(0.0f), m_farQuantumMS(0), m_quality(0), m_lodCount(0),
      m_sharedAnimation(NULL), m_isWoman(false), m_isMan(true), m_noFrustumCullingOnSpawn(false)
{
    // The original does not initialize speeds, effect, special ids or LOD distances here.
    m_model.Clear();
    Parse(node);
}

void CVisualCharacterType::Parse(TiXmlNode* node)
{
    m_name = CXmlHelper::GetAttributeValue(node, "name");
    XString empty;
    XString shared = CXmlHelper::GetAttributeValueOrUseDefault(node, "sharedSection", empty);
    if (!(shared == empty)) {
        m_sharedAnimation = NULL;
        for (int index = 0; index < m_sharedAnimations.size(); index++) {
            if (m_sharedAnimations[index]->m_name == shared) {
                m_sharedAnimation = m_sharedAnimations[index];
                break;
            }
        }
    }
    ParseInfo(node->FirstChild("info"));
    ParseSound(node->FirstChild("sound"));
    ParseModel(node->FirstChild("model"));
    node->FirstChild("sounds");
    TiXmlNode* lod = node->FirstChild("lod");
    if (lod != NULL) {
        ParseLods(lod);
    }
    TiXmlNode* disabled = node->FirstChild("picking_disabled_nodes");
    if (disabled != NULL) {
        for (TiXmlNode* item = disabled->FirstChild("node"); item != NULL; item = item->NextSibling("node")) {
            m_pickingDisabledNodes.addElement(CXmlHelper::GetAttributeValue(item, "name"));
        }
    }
}

CVisualCharacterType::~CVisualCharacterType()
{
    CleanupResources();
}

void CVisualCharacterType::CleanupResources() const
{
    m_model.Clear();
    for (int index = 0; index < m_modelCopies.size(); index++) {
        m_modelCopies[index].Clear();
    }
    m_modelCopies.removeAllElements();
    m_loaded = false;
}

void CVisualCharacterType::LoadCommonInfo(TiXmlNode* node)
{
    if (node != NULL) {
        m_defaultWalkSpeedMan = CXmlHelper::GetAttributeValue(node, "walk_speed_man").ToFloat();
        m_defaultWalkSpeedWoman = CXmlHelper::GetAttributeValue(node, "walk_speed_woman").ToFloat();
        m_defaultRunSpeedMan = CXmlHelper::GetAttributeValue(node, "run_speed_man").ToFloat();
        m_defaultRunSpeedWoman = CXmlHelper::GetAttributeValue(node, "run_speed_woman").ToFloat();
        m_defaultRunWoundSpeed = CXmlHelper::GetAttributeValue(node, "run_wound_speed").ToFloat();
        m_defaultCarrySpeed = CXmlHelper::GetAttributeValue(node, "carry_speed").ToFloat();
    }
}

void CVisualCharacterType::LoadSharedInfo(TiXmlNode* node)
{
    if (node != NULL) {
        SharedAnimation* animation = new SharedAnimation;
        animation->m_name = CXmlHelper::GetAttributeValue(node, "name");
        TiXmlNode* tracks = node->FirstChild("animation");
        if (tracks != NULL) {
            ParseAnimationTracks(tracks, animation);
        }
        m_sharedAnimations.addElement(animation);
    }
}

void CVisualCharacterType::ParseAnimationTracks(TiXmlNode* node, SharedAnimation* animation)
{
    // Original 0x13da30/0x13da4a directly treat this animation node as an element.
    TiXmlElement* source = static_cast<TiXmlElement*>(node);
    XString map(source->Attribute("map"));
    XString anm(source->Attribute("anm"));
    animation->m_mapResourceID = ((App*)WindowApp::m_instance)->m_resMap->getResourceID(map);
    animation->m_anmResourceID = ((App*)WindowApp::m_instance)->m_resMap->getResourceID(anm);
    ((App*)WindowApp::m_instance)->m_helper->LoadAnims(animation->m_animations, animation->m_anmResourceID);
    for (TiXmlNode* item = node->FirstChild(); item != NULL; item = item->NextSibling()) {
        TiXmlElement* element = NULL;
        if (item->Type() == TiXmlNode::TINYXML_ELEMENT) {
            element = static_cast<TiXmlElement*>(item);
        }
        if (element != NULL && XString(element->Value()) == XString("track")) {
            int type;
            element->QueryIntAttribute("id", &type);
            const char* name = element->Attribute("name");
            for (int index = 0; index < animation->m_animations.size(); index++) {
                if (CStrChar(animation->m_animations[index]->getName()) == CStrChar(name)) {
                    int trackIndex = index;
                    animation->m_tracks[type].addElement(trackIndex);
                    break;
                }
            }
        }
    }
}

void CVisualCharacterType::LoadCommonAnimation(TiXmlNode* node)
{
    if (node != NULL) {
        XString map = CXmlHelper::GetAttributeValue(node, "map");
        XString anm = CXmlHelper::GetAttributeValue(node, "anm");
        m_commonMapResourceID = ((App*)WindowApp::m_instance)->m_resMap->getResourceID(map);
        m_commonAnmResourceID = ((App*)WindowApp::m_instance)->m_resMap->getResourceID(anm);
        ((App*)WindowApp::m_instance)->m_helper->LoadAnims(m_commonAnimation, m_commonAnmResourceID);
    }
}

void CVisualCharacterType::DestroyCommonAnimation()
{
    SAFE_DELETE_VECTOR(m_commonAnimation);
    SAFE_DELETE_VECTOR(m_sharedAnimations);
}

CVisualCharacterType::Group CVisualCharacterType::GetCharacterModelCopy() const
{
    if (m_model == NULL) {
        LoadModel();
    }
    if (m_modelCopies.size() > 0) {
        Group copy = m_modelCopies[m_modelCopies.size() - 1];
        m_modelCopies.removeElementAt(m_modelCopies.size() - 1);
        return copy;
    }
    Group copy = SwerveHelper::FullObjectDuplicate(Group(m_model), 0);
    return copy;
}

void CVisualCharacterType::LoadModel() const
{
    if (m_loaded) {
        return;
    }
    LoadSounds();
    Object3D body = App::LoadObject3D(XString::AnsiString(m_bodyResource.m_data));
    if (body == NULL) {
        return;
    }
    Group head;
    if (m_headResource.Length() > 0) {
        head = Group(App::LoadObject3D(XString::AnsiString(m_headResource.m_data)));
        if (head == NULL) {
            return;
        }
    }
    if (head == Object3D()) {
        ReplaceTexture(body);
    }
    else {
        ReplaceTexture(body, head);
    }
    struct Callback {
        // Original 0x13f5b0: a true visitor result stops at the first group.
        static bool GroupCallback(Group&, void*) { return true; }
    };
    SwerveHelper::ForEachGroup(VisualNode(body), Callback::GroupCallback, NULL);
    const char* dummy = m_pBodyDummyName;
    if (m_rootDummy.GetLength() != 0) {
        dummy = m_rootDummy.GetCharPtr();
    }
    m_model = body.find(DGHelper::getSwerveID(dummy));
    if (m_model == NULL) {
        return;
    }
    Group duplicate(Group(((App*)WindowApp::m_instance)->m_visualCharacterTypeManager->m_commonGroup).duplicate());
    Group model(m_model);
    model.addChild(duplicate);
    bool hasHead = head != Object3D();
    for (unsigned int lod = 0; lod < m_lodCount; lod++) {
        // In the original LoadModel(0x13dd5c) loop the lookup is this+0x110+lod*0x10, i.e. m_headGroupBody[lod] (head bone group), not the LOD root group;
        // The head-swap model only replaces the meshes under the head bone, and the head hit sphere is also attached to the head bone (b143).
        Group headBone(body.find(DGHelper::getSwerveID(m_headGroupBody[lod].GetCharPtr())));
        if (headBone == NULL) {
            return;
        }
        if (hasHead) {
            if (m_headGroupBody[lod].GetLength() == 0 || m_headGroupHead[lod].GetLength() == 0) {
                continue;
            }
            Group headLevel(head.find(DGHelper::getSwerveID(m_headGroupHead[lod].GetCharPtr())));
            if (headLevel == NULL) {
                return;
            }
            Group parent(headLevel.getParent());
            if (parent != NULL) {
                parent.removeChild(headLevel);
            }
            for (int child = 0; child < headBone.getChildCount(); child++) {
                headBone.getChild(child).setRenderingEnable(false);
            }
            SwerveHelper::ResetCompositeTransform(headLevel);
            headBone.addChild(headLevel);
        }
        VisualMesh sphere(VisualWorld(((App*)WindowApp::m_instance)->m_helper->m_auxRender).find(DGHelper::getSwerveID("Sphere")));
        sphere = VisualMesh(sphere.duplicate());
        sphere.setUserID(GetHitzoneMeshUserId());
        Group hitZone(CSwerve::GetInstance()->m_swerve);
        hitZone.setUserID(GetHitzoneGroupUserId());
        VisualTransform transform(CSwerve::GetInstance()->m_swerve);
        transform.setIdentity();
        vec3 translation(0.2f, 0.0f, 0.0f);
        transform.postTranslate(&translation.m_x);
        vec3 scale(0.175f, 0.175f, 0.175f);
        transform.postScale(&scale.m_x);
        hitZone.setTransform(transform);
        hitZone.setPickingEnable(true);
        sphere.setPickingEnable(true);
        sphere.setRenderingEnable(false);
        hitZone.addChild(sphere);
        headBone.addChild(hitZone);
    }
    if (m_effect) {
        VisualWorld eyes;
        if (App::m_platformType == 0) {
            eyes = VisualWorld(App::LoadObject3D("BIN_M3G_BURNING_EYES_LOW"));
        }
        else {
            eyes = VisualWorld(App::LoadObject3D("BIN_M3G_BURNING_EYES_HIGH"));
        }
        Group eyesGroup(eyes.find(SwerveHelper::Id("eyes_BoneTransform")));
        for (unsigned int lod = 0; lod < m_lodCount; lod++) {
            Group copy(eyesGroup.duplicate());
            SwerveHelper::ResetCompositeTransform(copy);
            Group target(m_model.find(SwerveHelper::Id(m_headGroupBody[lod].GetCharPtr())));
            target.addChild(copy);
        }
    }
    for (int index = 0; index < m_pickingDisabledNodes.size(); index++) {
        VisualNode node(m_model.find(SwerveHelper::Id(XString::AnsiString(m_pickingDisabledNodes[index].m_data))));
        if (node != NULL) {
            node.setPickingEnable(false);
        }
    }
    VisualTransform transform(CSwerve::GetInstance()->m_swerve);
    SwerveHelper::CalculateBounds(m_bounds, VisualNode(m_model), transform);
    m_loaded = true;
}

void CVisualCharacterType::ReleaseCharacterModelCopy(Group group) const
{
    m_modelCopies.addElement(group);
}

const char* CVisualCharacterType::GetLodName(unsigned int lod) const
{
    return m_lodNames[lod].GetCharPtr();
}

float CVisualCharacterType::GetLodDistance(unsigned int lod) const
{
    return m_lodDistances[lod];
}

const char* CVisualCharacterType::GetLodHeadGroupName(unsigned int lod) const
{
    return m_headGroupBody[lod].GetCharPtr();
}

AnimationImpl* CVisualCharacterType::GetAnimation(EPersonAnimType type) const
{
    const Vector<int>& tracks = m_sharedAnimation->m_tracks[type];
    // Original reads the track count before CRandGen::GetInstance().
    int trackCount = tracks.size();
    int index = CRandGen::GetInstance()->GetRandRange(0, trackCount - 1);
    int animation = tracks[index];
    if (animation == -1) {
        return NULL;
    }
    return m_sharedAnimation->m_animations[animation];
}

void CVisualCharacterType::ParseInfo(TiXmlNode* node)
{
    if (node != NULL) {
        // The gender value is read into a local first (original frees the string before the compare).
        int gender = CXmlHelper::GetAttributeValue(node, "gender").ToInt();
        m_isMan = gender == 0;
        gender = CXmlHelper::GetAttributeValue(node, "gender").ToInt();
        m_isWoman = gender == 1;
        // ?: keeps the original operand order of the address select (if/else swaps it).
        float runSpeed = m_isMan ? m_defaultRunSpeedMan : m_defaultRunSpeedWoman;
        m_runSpeed = CXmlHelper::GetAttributeValueOrUseDefault(node, "runSpeed", XString(runSpeed, 2)).ToFloat();
        // ?: keeps the original operand order of the address select (if/else swaps it).
        float walkSpeed = m_isMan ? m_defaultWalkSpeedMan : m_defaultWalkSpeedWoman;
        m_walkSpeed = CXmlHelper::GetAttributeValueOrUseDefault(node, "walkSpeed", XString(walkSpeed, 2)).ToFloat();
        m_carrySpeed = CXmlHelper::GetAttributeValueOrUseDefault(node, "carrySpeed", XString(m_defaultCarrySpeed, 2)).ToFloat();
        m_runWoundSpeed = CXmlHelper::GetAttributeValueOrUseDefault(node, "runWoundSpeed", XString(m_defaultRunWoundSpeed, 2)).ToFloat();
        m_field6c = 0;
    }
}

void CVisualCharacterType::ParseSound(TiXmlNode* node)
{
    if (node != NULL) {
        m_exclamationSound = CXmlHelper::GetAttributeValue(node, "exclamation");
        m_attackSound = CXmlHelper::GetAttributeValue(node, "attack");
        m_woundSound = CXmlHelper::GetAttributeValue(node, "wound");
        m_deathSound = CXmlHelper::GetAttributeValue(node, "death");
        // These searches are present even though their results are unused (0x13e91a onward).
        if (m_exclamationSound.Length() > 0) {
            m_exclamationSound.Find(L"KEYSET", 0);
        }
        if (m_attackSound.Length() > 0) {
            m_attackSound.Find(L"KEYSET", 0);
        }
        if (m_woundSound.Length() > 0) {
            m_woundSound.Find(L"KEYSET", 0);
        }
        if (m_deathSound.Length() > 0) {
            m_deathSound.Find(L"KEYSET", 0);
        }
    }
}

void CVisualCharacterType::ParseModel(TiXmlNode* node)
{
    if (node == NULL) {
        return;
    }
    m_bodyResource = CXmlHelper::GetAttributeValue(node, "body");
    m_headResource = CXmlHelper::GetAttributeValue(node, "head");
    m_bodyTexture = CXmlHelper::GetAttributeValue(node, "bodyTexture");
    m_headTexture = CXmlHelper::GetAttributeValue(node, "headTexture");
    TiXmlElement* element = NULL;
    if (node->Type() == TiXmlNode::TINYXML_ELEMENT) {
        element = static_cast<TiXmlElement*>(node);
    }
    if (element != NULL) {
        if (element->Attribute("rootDummy") != NULL) {
            m_rootDummy = element->Attribute("rootDummy");
        }
        if (element->Attribute("bodyTextureNode") != NULL) {
            m_bodyTextureNode = element->Attribute("bodyTextureNode");
        }
        if (element->Attribute("legsTextureNode") != NULL) {
            m_legsTextureNode = element->Attribute("legsTextureNode");
        }
        if (element->Attribute("legsTexture") != NULL) {
            m_legsTexture = XString(element->Attribute("legsTexture"));
        }
        if (element->Attribute("headTextureNode") != NULL) {
            m_headTextureNode = element->Attribute("headTextureNode");
        }
        if (element->Attribute("lightTextureNode") != NULL) {
            m_lightTextureNode = element->Attribute("lightTextureNode");
        }
        if (element->Attribute("noFrustumCullingOnSpawn") != NULL) {
            m_noFrustumCullingOnSpawn = XString(element->Attribute("noFrustumCullingOnSpawn")).ToBool();
        }
        if (element->Attribute("effect") != NULL) {
            m_effect = XString(element->Attribute("effect")).ToBool();
        }
        else {
            m_effect = false;
        }
    }
}

void CVisualCharacterType::ParseLods(TiXmlNode* node)
{
    if (node == NULL || m_lodCount != 0) {
        return;
    }
    TiXmlElement* element = NULL;
    if (node->Type() == TiXmlNode::TINYXML_ELEMENT) {
        element = static_cast<TiXmlElement*>(node);
    }
    if (element != NULL && element->Attribute("nearDistance") != NULL &&
        element->Attribute("farDistance") != NULL && element->Attribute("farQuantumMS") != NULL &&
        element->Attribute("quality") != NULL) {
        double farDistance;
        double nearDistance;
        if (element->QueryDoubleAttribute("nearDistance", &nearDistance) == 0) {
            m_nearDistance = (float)nearDistance;
        }
        if (element->QueryDoubleAttribute("farDistance", &farDistance) == 0) {
            m_farDistance = (float)farDistance;
        }
        element->QueryIntAttribute("farQuantumMS", &m_farQuantumMS);
        element->QueryIntAttribute("quality", &m_quality);
        m_hasLodConfig = true;
    }
    for (TiXmlNode* item = node->FirstChild(); item != NULL; item = item->NextSibling()) {
        TiXmlElement* level = NULL;
        if (item->Type() == TiXmlNode::TINYXML_ELEMENT) {
            level = static_cast<TiXmlElement*>(item);
        }
        if (level != NULL && XString(level->Value()) == XString("level")) {
            // Original retains these attribute reads before the reads used for assignment.
            level->Attribute("name");
            level->Attribute("distance");
            level->Attribute("headGroupBody");
            level->Attribute("headGroupHead");
            m_lodNames[m_lodCount] = CStrChar(level->Attribute("name"));
            m_headGroupBody[m_lodCount] = CStrChar(level->Attribute("headGroupBody"));
            m_headGroupHead[m_lodCount] = CStrChar(level->Attribute("headGroupHead"));
            if (level->Attribute("SpecialHitZoneNode") != NULL) {
                m_specialHitZoneNodes[m_lodCount] = CStrChar(level->Attribute("SpecialHitZoneNode"));
                if (m_specialHitZoneNodes[m_lodCount].GetLength() != 0) {
                    m_specialHitZoneIds[m_lodCount] = DGHelper::getSwerveID(m_specialHitZoneNodes[m_lodCount].GetCharPtr());
                }
                else {
                    m_specialHitZoneIds[m_lodCount] = 0;
                }
            }
            double distance = 0.0;
            level->Attribute("distance", &distance);
            m_lodDistances[m_lodCount] = (float)distance;
            m_lodCount++;
            if (m_lodCount == 3) {
                break;
            }
        }
    }
}

void CVisualCharacterType::ReplaceTexture(Object3D body) const
{
    if (body == NULL) {
        return;
    }
    if (m_bodyTexture.Length() > 0) {
        VisualImage image(App::LoadObject3D(XString::AnsiString(m_bodyTexture.m_data)));
        if (image == NULL) {
            return;
        }
        const char* textureNode = NULL;
        if (m_bodyTextureNode.GetLength() != 0) {
            textureNode = m_bodyTextureNode.GetCharPtr();
        }
        DGHelper::ChangeImageRecursively(body, image, textureNode);
    }
    CSwerveGame* game = ((App*)WindowApp::m_instance)->m_helper->m_swerveGame;
    if (game != NULL && game->GetLightmap() != NULL) {
        VisualImage image = game->GetLightmap();
        const char* textureNode = NULL;
        if (m_lightTextureNode.GetLength() != 0) {
            textureNode = m_lightTextureNode.GetCharPtr();
        }
        DGHelper::ChangeImageRecursively(body, image, textureNode);
    }
    if (m_legsTextureNode.GetLength() != 0 && m_legsTexture.Length() > 0) {
        VisualImage image(App::LoadObject3D(XString::AnsiString(m_legsTexture.m_data)));
        if (image == NULL) {
            return;
        }
        const char* textureNode = NULL;
        if (m_legsTextureNode.GetLength() != 0) {
            textureNode = m_legsTextureNode.GetCharPtr();
        }
        DGHelper::ChangeImageRecursively(body, image, textureNode);
    }
}

void CVisualCharacterType::ReplaceTexture(Object3D body, Object3D head) const
{
    if (body == NULL || head == NULL) {
        return;
    }
    if (m_bodyTexture.Length() > 0 && m_headTexture.Length() > 0) {
        VisualImage bodyImage(App::LoadObject3D(XString::AnsiString(m_bodyTexture.m_data)));
        VisualImage headImage(App::LoadObject3D(XString::AnsiString(m_headTexture.m_data)));
        if (bodyImage == NULL || headImage == NULL) {
            return;
        }
        const char* bodyNode = NULL;
        if (m_bodyTextureNode.GetLength() != 0) {
            bodyNode = m_bodyTextureNode.GetCharPtr();
        }
        DGHelper::ChangeImageRecursively(body, bodyImage, bodyNode);
        const char* headNode = NULL;
        if (m_headTextureNode.GetLength() != 0) {
            headNode = m_headTextureNode.GetCharPtr();
        }
        DGHelper::ChangeImageRecursively(head, headImage, headNode);
    }
    CSwerveGame* game = ((App*)WindowApp::m_instance)->m_helper->m_swerveGame;
    if (game != NULL && game->GetLightmap() != NULL) {
        VisualImage image = game->GetLightmap();
        const char* textureNode = NULL;
        if (m_lightTextureNode.GetLength() != 0) {
            textureNode = m_lightTextureNode.GetCharPtr();
        }
        DGHelper::ChangeImageRecursively(body, image, textureNode);
        // Original reloads the filter after the first recursive replacement (0x13f37e).
        textureNode = NULL;
        if (m_lightTextureNode.GetLength() != 0) {
            textureNode = m_lightTextureNode.GetCharPtr();
        }
        DGHelper::ChangeImageRecursively(head, image, textureNode);
    }
    if (m_legsTextureNode.GetLength() != 0 && m_legsTexture.Length() > 0) {
        VisualImage image(App::LoadObject3D(XString::AnsiString(m_legsTexture.m_data)));
        if (image == NULL) {
            return;
        }
        const char* textureNode = NULL;
        if (m_legsTextureNode.GetLength() != 0) {
            textureNode = m_legsTextureNode.GetCharPtr();
        }
        DGHelper::ChangeImageRecursively(body, image, textureNode);
    }
}

void CVisualCharacterType::LoadSounds() const
{
    PreloadEventSound(m_exclamationSound);
    PreloadEventSound(m_attackSound);
    PreloadEventSound(m_woundSound);
    PreloadEventSound(m_deathSound);
}

void CVisualCharacterType::PreloadEventSound(XString sound) const
{
    if (sound.Length() < 1) {
        return;
    }
    if (sound.Find(L"KEYSET", 0) != -1) {
        ((App*)WindowApp::m_instance)->m_soundManager->PrecacheKeysetSounds(XString::AnsiString(sound.m_data));
    }
    else {
        XString filename = ((App*)WindowApp::m_instance)->m_resMap->getFilename(XString::AnsiString(sound.m_data));
        CDH_SoundManager::eSound id = (CDH_SoundManager::eSound)CDH_SoundManager::GetSoundFromResourceID(
            ((App*)WindowApp::m_instance)->m_resMap->getResourceID(filename));
        if (id != 0xa0) {
            ((App*)WindowApp::m_instance)->m_soundManager->PrecacheSound(id);
        }
        else {
            XString::AnsiString unused(sound.m_data);
        }
    }
}

int CVisualCharacterType::GetHitzoneMeshUserId()
{
    static int id = DGHelper::getSwerveID("ZombieHeadKillZone");
    return id;
}

int CVisualCharacterType::GetHitzoneGroupUserId()
{
    static int id = DGHelper::getSwerveID("ZombieHeadKillZoneTransform");
    return id;
}

int CVisualCharacterType::PlayEventSound(XString sound, float delay) const
{
    int id = 0xa0;
    if (sound.Length() < 1) {
        return id;
    }
    if (sound.Find(L"KEYSET", 0) != -1) {
        id = ((App*)WindowApp::m_instance)->m_soundManager->PlayRandomSoundWithDelay(XString::AnsiString(sound.m_data), delay, false);
    }
    else {
        XString filename = ((App*)WindowApp::m_instance)->m_resMap->getFilename(XString::AnsiString(sound.m_data));
        id = CDH_SoundManager::GetSoundFromResourceID(((App*)WindowApp::m_instance)->m_resMap->getResourceID(filename));
        if (id != 0xa0 && !((App*)WindowApp::m_instance)->m_soundManager->IsPlaying((CDH_SoundManager::eSound)id)) {
            ((App*)WindowApp::m_instance)->m_soundManager->Play((CDH_SoundManager::eSound)id, false, false);
        }
        else {
            XString::AnsiString unused(sound.m_data);
        }
    }
    return id;
}

int CVisualCharacterType::PlayAttackSound() const
{
    float delay = CRandGen::GetInstance()->GetRandRange(1000, 1500) * 0.001f;
    return PlayEventSound(m_attackSound, delay);
}

int CVisualCharacterType::PlayWoundSound() const
{
    float delay = CRandGen::GetInstance()->GetRandRange(500, 750) * 0.001f;
    return PlayEventSound(m_woundSound, delay);
}

int CVisualCharacterType::PlayDeathSound() const
{
    float delay = CRandGen::GetInstance()->GetRandRange(500, 750) * 0.001f;
    return PlayEventSound(m_deathSound, delay);
}



