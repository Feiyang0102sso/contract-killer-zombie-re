// Original u436: unit model ownership, animation/LOD, motion and hit testing.
#include "CUnitBody.h"
#include "CUnit.h"
#include "CUnitTarget.h"
#include "App.h"
#include "DGHelper.h"
#include "CSwerveGame.h"
#include "CUnitsController.h"
#include "CSceneObject.h"
#include "CBH_PersonType.h"
#include "CBH_PersonTypeManager.h"
#include "CVisualCharacterType.h"
#include "AnimatableSwerveObject.h"
#include "AnimationTransition.h"
#include "CDH_Animation.h"
#include "CDH_SoundManager.h"
#include "CCommonGameConfig.h"
#include "Ballistics.h"
#include "dgCamera.h"
#include "DGWorld.h"
#include "CRandGen.h"
#include "CGrenadeManager.h"
#include "CGrenade.h"
#include "CPowerUpManager.h"
#include "CPowerUp.h"
#include "CDH_BasicGameData.h"
#include "CBH_Player.h"
#include "CPlayerGameStats.h"
#include "CDH_Weapon.h"
#include <math.h>

using com::glu::platform::core::CRandGen;
typedef swerve::Object3D<CSwerveAssertingErrorHandler> Object3D;
typedef swerve::Group<CSwerveAssertingErrorHandler> Group;
typedef swerve::Node<CSwerveAssertingErrorHandler> Node;
typedef swerve::Transform<CSwerveAssertingErrorHandler> Transform;
namespace {
inline CSwerveGame* Game() { return ((App*)WindowApp::m_instance)->m_helper->m_swerveGame; }
inline float Length(const vec3& value)
{
    float square = value.LengthSqr();
    return MathLib::InvSqrt(square) * square;
}
inline vec3 UnitVector(const vec3& value) { return value * MathLib::InvSqrt(value.LengthSqr()); }
// Holds the scale object by value; the scale vector is local data of this inline operation.
inline void SetUniformScale(Group group, const float& value)
{
    vec3 scale(value, value, value);
    group.setScale(&scale.m_x);
}
// Keeps the fractional part; only the integer angle is taken modulo.
inline float NormalizeRotation(float angle)
{
    float integral;
    float fraction = modff(angle, &integral);
    float whole = (int)fabs((double)integral) % 360;
    if (angle < 0.0f) whole = 360.0f - whole;
    return whole + fraction;
}

}
const float CUnitBody::UNIT_BODY_RADIUS = 1.0f;
float CUnitBody::m_LodDistanceModifier = 1.0f;
const char* BOXES_LIGHT_TEXTURE_NODE_NAME = "gradient";

void ObstaclesContainer::AddStatic(const vec3& first, const vec3& second, const vec3& direction, float radius)
{
    Static obstacle;
    obstacle.m_first = first;
    obstacle.m_second = second;
    obstacle.m_direction = direction;
    obstacle.m_radius = radius;
    m_static.addElement(obstacle);
}

void ObstaclesContainer::Clear(bool dynamicObstacles, bool staticObstacles)
{
    if (dynamicObstacles) m_dynamic.removeAllElements();
    if (staticObstacles) m_static.removeAllElements();
}

CUnitBody::CUnitBody(const SSpawnablePerson& person, const IGameAIMap* map)
    : m_intersection(CSwerve::GetInstance()->m_swerve),
      m_intersection2(CSwerve::GetInstance()->m_swerve),
      m_personType(((App*)WindowApp::m_instance)->m_personTypeManager->GetType(person.m_typeName)),
      m_visualType(m_personType->GetVisualObject(person.m_seed)), m_map(map),
      m_scratch(SwerveHelper::NewTransform())
{
    m_orientationControl = ORIENTATION_NONE;
    m_ignoreSteering = false;
    m_speed = 0.0f;
    m_cullSphere = NULL;
    m_owner = NULL;
    m_animatable = NULL;
    m_transition = NULL;



    m_field138 = 0;
    m_field13c = 0;
    __builtin_memset(&m_location, 0, sizeof(m_location));
    m_muzzleFlash = NULL;
    m_animation = (EPersonAnimType)37;
    m_spawnPointId = -1;
    m_previousPointId = -1;
    m_transformDirty = true;
    m_orientation = vec3(0.0f, -1.0f, 0.0f);
    m_yaw = 0.0f;
    m_field1b0 = 0.0f;
    m_ellipsoid.m_radii = vec3(0.3f, 0.3f, 0.9f);
    m_refreshBounds = false;
    m_boundsTimer = 0;
    m_stepSpeed = 0.0f;
    m_cameraPitch = 1;
    m_field230 = 1;
    m_flag234 = false;
    m_flag235 = false;
    m_visible = false;
    m_beheaded = false;
    m_spawning = true;
    m_flag239 = false;
    m_scale = 1.0f;
    m_pickDistance = 0.0f;
    m_updateFrameCounter = 0;
    m_seed = person.m_seed;
    m_turnRate = 0.2f;
    m_lodCount = m_visualType->m_lodCount;
    m_currentLod = 0;
    m_cameraDistance = 0.0f;
    m_animationElapsed = 0;
    m_animationQuantum = 0;
    m_forcedLod = -1;
    m_weaponShown = false;
    m_weaponLoaded = false;
    m_soundCooldown = 0;
    m_field310 = NULL;
    m_field314 = NULL;
    m_field318 = NULL;
    m_body = this; // CKinematicBody virtual base back pointer (+0x320).
    m_transform = SwerveHelper::NewTransform();
    for (int index = 0; index < 5; ++index) m_lighting[index] = NULL;
    m_model = m_visualType->GetCharacterModelCopy();
    m_scale = m_personType->GenerateRandScale();
    vec3 scale(m_scale, m_scale, m_scale);
    m_model.setScale(&scale.m_x);
    m_cullSphere = new FrustumCulling::CullSphere(m_model, m_scale);
    m_root = Group(m_model.find(DGHelper::getSwerveID("down_barTransform")));
    m_autoAnimationLod = m_lodCount > 1;
    for (unsigned int lod = 0; lod < m_lodCount; ++lod) {
        m_lodDistances[lod] = m_visualType->GetLodDistance(lod);
        m_lods[lod] = Group(m_model.find(DGHelper::getSwerveID(m_visualType->GetLodName(lod))));
        m_lods[lod].setRenderingEnable(lod == m_currentLod);
    }
    if (m_visualType->m_quality != 0) {
        int device = App::GetDeviceType();
        if ((unsigned int)(device - 5) < 2 || device == 11) {
            for (unsigned int lod = 0; lod < m_lodCount; ++lod) m_lodDistances[lod] += 40.0f;
        } else if (device == 12) {
            for (unsigned int lod = 0; lod < m_lodCount; ++lod) m_lodDistances[lod] += 120.0f;
            m_autoAnimationLod = false;
        }
    }
    if (m_root.m_interface == NULL) m_root = Group(m_model.find(DGHelper::getSwerveID("rootTransform")));
    InitBodyZones();
    LoadWeapon();
    ShowWeapon(true);
    __builtin_memset(m_animations, 0, sizeof(m_animations));
    m_state = UNIT_STATE_INITIAL;
    m_hitRayOrigin = vec3();
    m_hitRayDirection = vec3();
    m_hitRayFraction = 0.0f;
    m_bounds.m_min = m_visualType->m_bounds.m_min * m_scale;
    m_bounds.m_max = m_visualType->m_bounds.m_max * m_scale;
    m_height = m_bounds.m_max.m_z - m_bounds.m_min.m_z;
    if (m_personType->m_isBoss) m_refreshBounds = true;
    Game()->GetCollision()->AddKinematicBody(this);
}

void CUnitBody::InitBodyZones()
{
    if (m_personType == NULL || m_model.m_interface == NULL) return;
    m_headKillZone = Group(m_model.find(DGHelper::getSwerveID("head_kill_zoneTransform")));
    m_headArmorZone = Group(m_model.find(DGHelper::getSwerveID("head_armor_zoneTransform")));
    m_bodyVitalZone = Group(m_model.find(DGHelper::getSwerveID("body_vital_zoneTransform")));
    m_bodyArmorZone = Group(m_model.find(DGHelper::getSwerveID("body_armor_zoneTransform")));
    int headId = DGHelper::getSwerveID("ZombieHeadKillZoneTransform");
    for (unsigned int lod = 0; lod < m_lodCount; ++lod) {
        m_heads[lod] = Group(m_lods[lod].find(headId));
        int specialId = m_visualType->m_specialHitZoneIds[lod];
        if (specialId != 0) {
            Node mesh(m_lods[lod].find(specialId));
            if (mesh.m_interface != NULL) {
                mesh.setPickingEnable(true);
                Group parent(mesh.getParent());
                if (parent.m_interface != NULL) parent.setRenderingEnable(false);
            }
        }
    }
    if (m_headKillZone.m_interface != NULL) m_headKillZone.setRenderingEnable(true);
    if (m_headArmorZone.m_interface != NULL) m_headArmorZone.setRenderingEnable(false);
    if (m_bodyVitalZone.m_interface != NULL) m_bodyVitalZone.setRenderingEnable(false);
    if (m_bodyArmorZone.m_interface != NULL) {
        m_bodyArmorZone.setRenderingEnable(false);
        m_bodyArmorZone.setPickingEnable(m_visualType->m_isWoman);
    }
}

void CUnitBody::LoadWeapon()
{
    if (m_weaponLoaded || m_personType->m_weapon == NULL) return;
    swerve::Image2D<CSwerveAssertingErrorHandler> lightmap = Game()->GetLightmap();
    for (unsigned int lod = 0; lod < m_lodCount; ++lod) {
        if (m_weapons[lod].m_interface == NULL) {
            m_weapons[lod] = m_personType->m_weapon->GetModelCopy();
            SwerveHelper::ResetCompositeTransform(m_weapons[lod]);
            DGHelper::ChangeImageRecursively(m_weapons[lod], lightmap, XString::AnsiString(m_personType->m_weapon->m_lightNode.c_str()));
            AddLightingController(m_weapons[lod]);
            m_weapons[lod].setPickingEnable(true);
        }
    }
    if (m_muzzleFlash == NULL) {
        bool haveController = CDH_Animation::HaveController(m_weapons[0], "shotflash");
        if (haveController) {
            m_muzzleFlash = new CDH_Animation(m_weapons[0]);
            m_muzzleFlash->addController(XString("shotflash"));
        }
    }
    BindWeaponToHand();
    m_weaponLoaded = true;
}

void CUnitBody::ShowWeapon(bool show)
{
    if (!m_weaponLoaded) LoadWeapon();
    m_weaponShown = show;
    for (unsigned int lod = 0; lod < m_lodCount; ++lod) {
        if (m_weapons[lod].m_interface != NULL) m_weapons[lod].setRenderingEnable(show);
    }
}

CUnitBody::~CUnitBody()
{
    Game()->GetCollision()->RemoveKinematicBody(this);
    m_model.setRenderingEnable(true);
    m_model.setPickingEnable(true);
    if (m_beheaded) {
        unsigned int count = m_visualType->m_lodCount;
        for (unsigned int lod = 0; lod != count; ++lod) {
            Group head(m_model.find(DGHelper::getSwerveID(m_visualType->GetLodHeadGroupName(lod))));
            head.setRenderingEnable(true);
        }
    }
    Cleanup();
}

void CUnitBody::Cleanup()
{
    RemoveWeapon();
    UnbindFromWorld();
    if (m_cullSphere != NULL) { delete m_cullSphere; m_cullSphere = NULL; }
    // The original Releases and nulls each one in turn, not through assignment of an empty temporary.
    m_skin.Clear();
    m_root.Clear();
    m_visualType->ReleaseCharacterModelCopy(m_model);
    m_model.Clear();
    if (m_box.m_interface != NULL) m_box.Clear();
    m_headKillZone.Clear();
    m_headArmorZone.Clear();
    m_bodyVitalZone.Clear();
    m_bodyArmorZone.Clear();
    m_intersection.Clear();
    m_icon.Clear();
    for (unsigned int lod = 0; lod < m_lodCount; ++lod) {
        m_lods[lod].Clear();
        m_lodChannels[lod].Clear();
    }
    m_intersection2.Clear();
    if (m_animatable != NULL) { delete m_animatable; m_animatable = NULL; }
    if (m_transition != NULL) { delete m_transition; m_transition = NULL; }
    for (int index = 0; index < 5; ++index) {
        if (m_lighting[index] != NULL) { delete m_lighting[index]; m_lighting[index] = NULL; }
    }
    if (m_muzzleFlash != NULL) { delete m_muzzleFlash; m_muzzleFlash = NULL; }
}

void CUnitBody::Init(CUnit* unit, const XString& point)
{
    m_owner = unit;
    m_spawnPoint = point;
    LoadSkin();
    InitLODs();
    m_transition = new AnimationTransition(m_animatable, this);
    SetToPoint(point);
}

void CUnitBody::LoadSkin()
{
    const char* mapResource = CVisualCharacterType::m_commonMapResourceID;
    if (m_visualType->m_sharedAnimation != NULL && m_visualType->m_sharedAnimation->m_mapResourceID != NULL) {
        mapResource = m_visualType->m_sharedAnimation->m_mapResourceID;
    }
    AddLightingController(m_model);
    ((App*)WindowApp::m_instance)->m_helper->CreateAnimatableObject(m_model, &m_animatable, mapResource);
}

void CUnitBody::InitLODs()
{
    for (unsigned int lod = 0; lod < m_lodCount; ++lod) {
        InitLODChannel(lod, m_lods[lod]);
        int count = m_lodChannels[lod].Size();
        for (int index = 0; index != count; ++index) {
            m_animatable->m_channelBlended[m_lodChannels[lod][index]] = lod == m_currentLod;
        }
    }
}

void CUnitBody::SetToPoint(const XString& point)
{
    m_spawnPointId = m_map->GetPointIDByName(point);
    if (m_spawnPointId == -1) return;
    m_position = m_map->GetMapPointPostition(m_spawnPointId);
    CSceneObject* object = Game()->GetUnitsController()->GetSceneObjectInPoint(point);
    if (object != NULL) m_position = m_position + object->GetSpawnPositionShift();
    m_ellipsoid.m_position = m_position + vec3(0.0f, 0.0f, m_ellipsoid.m_radii.m_z);
    const float* orientation = m_map->GetMapPointOrientation(m_spawnPointId);
    Transform transform(CSwerve::GetInstance()->m_swerve);
    transform.postRotate(orientation);
    float matrix[16];
    transform.get(matrix);
    m_forward.m_x = matrix[1];
    m_forward.m_y = matrix[5];
    m_yaw = MathLib::ArcTan(m_forward.m_y, -m_forward.m_x);
    m_transformDirty = true;
    m_map->SyncLocation(m_location, m_position);
    m_position = m_location.m_position;
}

void CUnitBody::Spawn() { BindToWorld(); }

void CUnitBody::BindToWorld()
{
    Group world(Game()->m_cameraNode.find(DGHelper::getSwerveID("worldTransform")));
    world.addChild(m_model);
}

void CUnitBody::SyncLocation()
{
    m_map->SyncLocation(m_location, m_position);
    m_position = m_location.m_position;
}

void CUnitBody::Tick(int elapsed)
{
    m_soundCooldown -= WindowApp::m_instance->m_updateFrameTime;
    if (m_soundCooldown < 0) m_soundCooldown = 0;
    if (m_transformDirty) { CalculateStep(elapsed); UpdateTransform(elapsed); }
    UpdateAnimation(elapsed);
    m_map->SyncLocation(m_location, m_position);
    m_position = m_location.m_position;
    if (m_refreshBounds) {
        if (m_boundsTimer < 250) {
            m_boundsTimer += elapsed;
        } else {
            m_boundsTimer = 0;
            Bounds3 bounds;
            Transform transform(CSwerve::GetInstance()->m_swerve);
            SwerveHelper::CalculateBounds(bounds, m_model, transform);
            m_bounds.m_min = bounds.m_min * m_scale;
            m_bounds.m_max = bounds.m_max * m_scale;
        }
    }
}

void CUnitBody::CalculateStep(int elapsed)
{
    float movementSpeed = GetMovementSpeed();
    m_speed = Length(m_taskForce + m_steeringForce);
    float seconds = (float)elapsed / 1000.0f;
    if (m_ignoreSteering || m_state == 1 || m_state == 4 || m_state == 5) {
        m_steeringForce = vec3();
        m_taskForce = vec3();
    }
    m_owner->IsCivilian(); // Original unused getter call remains observable.
    vec3 velocity = (m_taskForce + m_steeringForce) * movementSpeed;
    float length = Length(velocity);
    if (length > movementSpeed) velocity = velocity * (movementSpeed / length); // Original BLE skips unordered as well.
    m_position = m_position + velocity * seconds;
    m_orientation = vec3(m_forward.m_x, m_forward.m_y, 0.0f);
    m_stepSpeed = 0.0f;
    if (m_orientationControl == ORIENTATION_MOVEMENT) {
        if (m_speed > 0.0f) CalculateRotation(m_yaw, m_turnRate, (float)elapsed, velocity);
    } else if (m_orientationControl == ORIENTATION_INSTANT) {
        m_yaw = MathLib::ArcTan(m_orientation.m_x, m_orientation.m_y);
    } else if (m_orientationControl == ORIENTATION_TURN) {
        if (CalculateRotation(m_yaw, m_turnRate, (float)elapsed, m_forward) && m_turning) {
            m_turning = false;
            m_owner->m_mind.OnTurningComplete();
            if (m_state == 3) m_state = UNIT_STATE_INITIAL;
        }
    }
}

void CUnitBody::UpdateTransform(int elapsed)
{
    if (!Game()->IsNavMeshEnabled()) {
        float milliseconds = (float)elapsed;
        vec3 target;
        target.m_x = m_ellipsoid.m_position.m_x + ((m_position.m_x + 0.0f) - m_ellipsoid.m_position.m_x);
        target.m_y = m_ellipsoid.m_position.m_y + ((m_position.m_y + 0.0f) - m_ellipsoid.m_position.m_y);
        target.m_z = m_ellipsoid.m_position.m_z + milliseconds * 0.001f;
        CCollision* collision = Game()->GetCollision();
        collision->MoveTo(m_ellipsoid, target, false, -1);
        target.m_x = m_ellipsoid.m_position.m_x + milliseconds * 0.0f * 0.001f;
        target.m_y = m_ellipsoid.m_position.m_y + milliseconds * 0.0f * 0.001f;
        target.m_z = m_ellipsoid.m_position.m_z + milliseconds * -3.0f * 0.001f;
        collision->MoveTo(m_ellipsoid, target, false, -1);
        m_position = m_ellipsoid.m_position;
        m_position.m_z -= m_ellipsoid.m_radii.m_z;
    }
    if (m_position.m_x != m_position.m_x || m_position.m_y != m_position.m_y ||
        m_position.m_z != m_position.m_z || m_yaw != m_yaw) return;
    Transform transform(CSwerve::GetInstance()->m_swerve);
    m_steeringForce = vec3();
    m_taskForce = vec3();
    float orientation[4];
    float quaternion[4];
    m_model.getOrientation(orientation);
    m_model.getOrientation84(quaternion);
    Transform unused = SwerveHelper::NewTransform();
    m_model.getTransformTo(Game()->m_cameraNode, unused);
    float matrix[16];
    transform.get(matrix); // Original output is unused, but the getter is retained.
    vec3 scale;
    m_model.getScale(&scale.m_x);
    vec3 position(m_position.m_x / scale.m_x, m_position.m_y / scale.m_y, m_position.m_z / scale.m_z);
    transform.postTranslate(&position.m_x);
    float rotation[4] = {180.0f - m_yaw, 0.0f, 0.0f, 1.0f};
    transform.postRotate(rotation);
    m_model.setTransform(transform);
    transform = Transform(); // Original releases this handle before LOD selection.
    m_transformDirty = false;
    if (m_lodCount != 0) {
        m_cameraDistance = Length(Game()->GetCurrentCameraPos() - m_position);
        float distance = m_cameraDistance * m_LodDistanceModifier;
        unsigned int selected = m_lodCount - 1;
        // original 104ec8 BPL also selects the current entry when unordered; only looks backward on an ordered less-than.
        // With only one initial entry the 0 threshold is still compared, but after decrementing to 0 the threshold is not read again.
        do {
            if (!(distance < m_lodDistances[selected])) break;
            if (selected == 0) break;
            --selected;
        } while (selected != 0);
        int platform = App::m_platformType;
        Game()->m_camera->GetFOV(); // Kept even though its return value is unused.
        if (platform == 0 && selected == 0) selected = 1;
        if (m_forcedLod >= 0) selected = m_forcedLod;
        if (m_currentLod != selected) {
            int count = m_lodChannels[m_currentLod].Size();
            for (int index = 0; index != count; ++index) {
                m_animatable->m_channelBlended[m_lodChannels[m_currentLod][index]] = false;
            }
            m_lods[m_currentLod].setRenderingEnable(false);
            m_currentLod = selected;
            m_lods[selected].setRenderingEnable(true);
            count = m_lodChannels[m_currentLod].Size();
            for (int index = 0; index != count; ++index) {
                m_animatable->m_channelBlended[m_lodChannels[m_currentLod][index]] = true;
            }
        }
    }
}

void CUnitBody::UpdateAnimation(int elapsed)
{
    if (m_visualType->m_hasLodConfig) {
        int accumulated = m_animationElapsed + elapsed;
        m_animationElapsed = accumulated;
        if (accumulated < m_animationQuantum) return;
        if (m_autoAnimationLod) elapsed = m_visualType->GetAnimLodQuantumMS(m_cameraDistance, elapsed);
        m_animationElapsed = accumulated - elapsed;
        m_animationQuantum = elapsed;
    }
    if (m_transition != NULL) m_transition->update(elapsed << 12);
    if (m_animatable == NULL) return;
    if (!Game()->IsAnimationUpdateEnabled() && m_visible) {
        if (App::m_platformType != 0) {
            m_animatable->animate();
        } else {
            ++m_updateFrameCounter;
            if (m_updateFrameCounter == 3) {
                m_updateFrameCounter = 0;
                if (m_animation != 0) m_animatable->animate();
            }
        }
    }
    m_animatable->advanceTime((float)elapsed);
}

void CUnitBody::SetWorldPosition(const vec3& position)
{
    m_position = position;
    m_ellipsoid.m_position = m_position + vec3(0.0f, 0.0f, m_ellipsoid.m_radii.m_z);
    m_transformDirty = false;
    m_map->SyncLocation(m_location, m_position);
    m_position = m_location.m_position;
    UpdateTransform(0);
}

void CUnitBody::OnNeutralization() { UnbindWeapon(); UnbindAllIcons(); }

void CUnitBody::UnbindWeapon()
{
    for (unsigned int lod = 0; lod < m_lodCount; ++lod) {
        XString name = XString::Format(L"weapon_lod%dTransform", lod + 1);
        Group parent(m_model.find(DGHelper::getSwerveID(XString::AnsiString(name.c_str()))));
        if (parent.m_interface != NULL && m_weapons[lod].m_interface != NULL) parent.removeChild(m_weapons[lod]);
    }
}

void CUnitBody::UnbindAllIcons()
{
    const char* name = CVisualCharacterType::m_pBodyDummyName;
    if (m_visualType->m_rootDummy.GetLength() != 0) name = m_visualType->m_rootDummy.GetCharPtr();
    Group parent(m_model.find(DGHelper::getSwerveID(name)));
    if (parent.m_interface != NULL && m_icon.m_interface != NULL) {
        parent.removeChild(m_icon);
        m_icon.Clear();
    }
}

void CUnitBody::CheckVisibility(const FrustumCulling& frustum)
{
    m_visible = false;
    if (m_cullSphere == NULL) return;
    if (m_spawning && m_visualType->m_noFrustumCullingOnSpawn) {
        m_visible = true;
    } else {
        m_cullSphere->m_offset.x = m_position.m_x;
        m_cullSphere->m_offset.y = m_position.m_y;
        m_cullSphere->m_offset.z = m_position.m_z;
        m_visible = frustum.CheckVisibility(*m_cullSphere);
    }
}

void CUnitBody::DefineCameraPitch(const vec3& direction)
{
    vec3 horizontal(direction.m_x, direction.m_y, 0.0f);
    vec3 normalized = UnitVector(direction);
    horizontal = UnitVector(horizontal);
    float dot = normalized.Dot(horizontal);
    m_cameraPitch = !(dot < 0.86f); // Original BPL includes unordered.
}

void CUnitBody::SetOrientationToPoint(const vec3& point, bool instant, float rate)
{
    vec3 direction = point - m_position;
    if (direction.LengthSqr() > 0.01f) m_forward = direction;
    m_turnRate = rate;
    DefineCameraPitch(direction);
    direction.m_z = 0.0f;
    m_orientation = UnitVector(direction);
    m_turning = true;
    if (instant) {
        m_yaw = MathLib::ArcTan(m_forward.m_x, m_forward.m_y);
        m_orientationControl = ORIENTATION_NONE;
    } else {
        m_state = (UnitState)3;
        m_orientationControl = ORIENTATION_TURN;
    }
    m_transformDirty = true;
}

void CUnitBody::GetCurrentPosition(int* current, int* previous)
{
    *current = m_location.m_pointId;
    *previous = m_previousPointId;
}

void CUnitBody::MakeStep(int, const vec3& point, float speed)
{
    vec3 direction = point - m_position;
    if (direction.LengthSqr() > 0.0001f) {
        m_stepSpeed = speed;
        m_forward = UnitVector(direction);
        m_transformDirty = true;
    } else {
        m_stepSpeed = 0.0f;
        m_forward = vec3(1.0f, 0.0f, 0.0f);
        m_transformDirty = false;
    }
    m_state = (UnitState)2;
}

void CUnitBody::SetSteeringForce(const vec3& force) { m_steeringForce = force; m_transformDirty = true; }
void CUnitBody::SetTaskForce(const vec3& force) { m_taskForce = force; m_transformDirty = true; }
void CUnitBody::SetOrientationControl(OrientationControl control) { m_orientationControl = control; }
vec3 CUnitBody::GetSpeed() const { return m_forward * m_stepSpeed; }
float CUnitBody::GetRadius() const { return UNIT_BODY_RADIUS; }

void CUnitBody::Behead()
{
    unsigned int chance = CRandGen::GetInstance()->GetRand(0xffffffffu) % 100;
    unsigned int count = m_visualType->m_lodCount;
    for (unsigned int lod = 0; lod != count; ++lod) {
        Group head(m_model.find(DGHelper::getSwerveID(m_visualType->GetLodHeadGroupName(lod))));
        if (chance <= 19 && lod == m_currentLod) {
            Group copy(head.duplicate());
            Group parent(copy.getParent());
            if (parent.m_interface != NULL) parent.removeChild(copy);
            copy.setRenderingEnable(true);
            copy.setPickingEnable(false);
            Group world(Game()->m_cameraNode.find(SwerveHelper::Id("worldTransform")));
            world.addChild(copy);
            head.getTransformTo(world, m_scratch);
            float originalMatrix[16];
            m_scratch.get(originalMatrix);
            copy.setTransform(m_scratch);
            copy.getTransformTo(world, m_scratch);
            float duplicateMatrix[16];
            m_scratch.get(duplicateMatrix);
            vec3 shift(originalMatrix[3] - duplicateMatrix[3], originalMatrix[7] - duplicateMatrix[7], originalMatrix[11] - duplicateMatrix[11]);
            copy.translate(&shift.m_x);
            vec3 position(originalMatrix[3], originalMatrix[7], originalMatrix[11]);
            vec3 velocity = UnitVector(position - Game()->GetCurrentCameraPos()) * 3.0f;
            velocity.m_z = Length(velocity);
            Game()->GetSwerveBallistics()->AddProjectile(copy, position, velocity, 5.0f);
        }
        head.setRenderingEnable(false);
    }
    DrawHitEffect(HIT_EFFECT_HEAD);
    m_beheaded = true;
}

void CUnitBody::DrawHitEffect(EHitEffect effect)
{
    vec3 normal = m_hitNormal;
    vec3 direction = m_hitRayDirection;
    vec3 point = m_hitRayOrigin + direction * m_hitRayFraction;
    m_lods[m_currentLod].getTransformTo(Game()->m_cameraNode, m_scratch);
    SwerveHelper::TransformPoint(&point, &point, m_scratch);
    SwerveHelper::TransformVector(&direction, &direction, m_scratch);
    SwerveHelper::TransformVector(&normal, &normal, m_scratch);
    normal = UnitVector(normal);
    switch (effect) {
    case HIT_EFFECT_BODY: Game()->GetDGWorld()->bodyHit(point, normal); break;
    case HIT_EFFECT_HEAD: Game()->GetDGWorld()->headBlow(point, normal); break;
    case HIT_EFFECT_HEAL: Game()->GetDGWorld()->healBlast(point, normal); break;
    case HIT_EFFECT_SLIME: Game()->GetDGWorld()->slime_blast(point, normal); break;
    case HIT_EFFECT_RICOCHET: {
        Game()->AddRicochet(point, normal * 25.0f, 5.0f, 0);
        vec3 cameraDirection = UnitVector(Game()->GetCurrentCameraPos() - point);
        vec3 velocity = cameraDirection * 0.01f;
        vec3 flashPoint = point + cameraDirection * 0.1f;
        Game()->AddFlash(flashPoint, velocity, 5.0f);
        break;
    }
    default: break;
    }
}

void CUnitBody::Blow()
{
    m_model.getCompositeTransform(m_scratch);
    float matrix[16];
    m_scratch.get(matrix);
    vec3 point(matrix[3], matrix[7], matrix[11]);
    vec3 direction(matrix[8], matrix[9], matrix[10]);
    Game()->GetDGWorld()->bodyBlow(point, direction);
    m_model.setRenderingEnable(false);
    unsigned int sound = CRandGen::GetInstance()->GetRand(100);
    if ((sound & 1) != 0) ((App*)WindowApp::m_instance)->m_soundManager->Play((CDH_SoundManager::eSound)25, false, false);
    else ((App*)WindowApp::m_instance)->m_soundManager->Play((CDH_SoundManager::eSound)26, false, false);
    Behead();
}

void CUnitBody::SetWorldPositionToMassCenter() {} // Original 0x105d88 is bx lr+nop.
void CUnitBody::StopAllAnimations(int fade)
{
    m_animationElapsed = m_animationQuantum;
    if (m_transition != NULL) m_transition->stopAllAnims(fade);
}
void CUnitBody::IgnoreSteering(bool ignore) { m_ignoreSteering = ignore; }

WeakIntrusivePtr<AnimationInstance> CUnitBody::StartAnimation(EPersonAnimType animation, bool loop)
{
    m_animationElapsed = m_animationQuantum;
    if (m_personType == NULL || m_transition == NULL) {
        return WeakIntrusivePtr<AnimationInstance>(NULL);
    }
    if ((unsigned int)animation >= 37) {
        return WeakIntrusivePtr<AnimationInstance>(NULL);
    }
    AnimationImpl* implementation = m_visualType->GetAnimation(animation);
    if (implementation == NULL) {
        return WeakIntrusivePtr<AnimationInstance>(NULL);
    }
    m_animation = animation;
    return m_transition->startAnimation(implementation, 1.0f, loop, 1000, 1.0f, 0.0f);
}

WeakIntrusivePtr<AnimationInstance> CUnitBody::PlayAfraidAnimation()
{
    m_animationElapsed = m_animationQuantum;
    return StartAnimation((EPersonAnimType)1, false);
}

WeakIntrusivePtr<AnimationInstance> CUnitBody::PlayDeathAnimation(bool special)
{
    m_animationElapsed = m_animationQuantum;
    if (special) {
        unsigned int chance = CRandGen::GetInstance()->GetRand(100);
        // ?: needed: original sets 25 first then movhi 24 (two returns give cmp #0x32 / movhs).
        EPersonAnimType animation = chance > 49 ? (EPersonAnimType)24 : (EPersonAnimType)25;
        return StartAnimation(animation, false);
    }
    if (m_spawning && m_map->GetPointSpawnType(m_spawnPointId) == 1) return StartAnimation((EPersonAnimType)21, false);
    if (m_flag239) return StartAnimation((EPersonAnimType)22, false);
    if (WindowApp::m_instance->GetRandomValue().Int(2) == 0) {
        float sourceX = m_attackSource.m_x;
        float sourceY = m_attackSource.m_y;
        float positionX = m_position.m_x;
        float positionY = m_position.m_y;
        vec3 orientation = m_orientation;
        SetOrientationToPoint(m_attackSource, true, 1.0f);
        float dot = (sourceX - positionX) * orientation.m_x + (sourceY - positionY) * orientation.m_y + orientation.m_z * 0.0f;
        if (dot > 0.0f) return StartAnimation((EPersonAnimType)12, false);
        return StartAnimation((EPersonAnimType)13, false);
    }
    return StartAnimation((EPersonAnimType)23, false);
}

EPersonAnimType CUnitBody::GetSpawnAnimation()
{
    if (m_map->GetPointSpawnType(m_spawnPointId) == 1) return (EPersonAnimType)14;
    return (EPersonAnimType)-1;
}

WeakIntrusivePtr<AnimationInstance> CUnitBody::PlayShotAnimation()
{
    m_animationElapsed = m_animationQuantum;
    if (m_personType == NULL || m_transition == NULL) {
        return WeakIntrusivePtr<AnimationInstance>(NULL);
    }
    EPersonAnimType animation = m_owner->GetAnimWeapon();
    AnimationImpl* implementation = m_visualType->GetAnimation(animation);
    if (implementation == NULL) {
        return WeakIntrusivePtr<AnimationInstance>(NULL);
    }
    m_animation = animation;
    return m_transition->startAnimation(implementation, 1.0f, false, 1000, 1.0f, 0.0f);
}

void CUnitBody::ThrowWeapon()
{
    if (!m_weaponLoaded) return;
    vec3 position;
    if (m_weapons[0].m_interface != NULL) {
        Transform transform(CSwerve::GetInstance()->m_swerve);
        m_weapons[0].getTransformTo(Game()->m_cameraNode, transform);
        DGHelper::GetPosition(transform, position);
    }
    RemoveWeapon();
    Group weapon = m_personType->m_weapon->GetModelCopy();
    if (weapon.m_interface == NULL) return;
    SetUniformScale(weapon, m_scale);
    swerve::Image2D<CSwerveAssertingErrorHandler> lightmap = Game()->GetLightmap();
    DGHelper::ChangeImageRecursively(weapon, lightmap, XString::AnsiString(m_personType->m_weapon->m_lightNode.c_str()));
    Game()->LaunchUnitWeapon(position, weapon, m_owner->m_weaponHp);
}

void CUnitBody::RemoveWeapon()
{
    if (!m_weaponLoaded) return;
    UnbindWeapon();
    for (unsigned int lod = 0; lod < m_lodCount; ++lod) {
        if (m_weapons[lod].m_interface != NULL) {
            RemoveLightingController(m_weapons[lod]);
            m_weapons[lod].Clear();
            m_personType->m_weapon->ReleaseModelCopy();
        }
    }
    if (m_muzzleFlash != NULL) { delete m_muzzleFlash; m_muzzleFlash = NULL; }
    m_weaponLoaded = false;
    m_weaponShown = false;
}

bool CUnitBody::IsRunning() const { return m_animation == 3; }
void CUnitBody::PlayWeaponSound()
{
    if (m_personType->m_weapon != NULL) ((App*)WindowApp::m_instance)->m_soundManager->Play((CDH_SoundManager::eSound)m_personType->m_weapon->m_shootSound, false, false);
}
void CUnitBody::PlayAttackSound(float cooldown)
{
    if (m_soundCooldown > 0) return;
    int delay = (int)cooldown;
    if (m_visualType->PlayAttackSound() == 160) delay = 0;
    m_soundCooldown = delay;
}
void CUnitBody::PlayDeathSound(float cooldown)
{
    if (m_soundCooldown > 0) return;
    int delay = (int)cooldown;
    if (m_visualType->PlayDeathSound() == 160) delay = 0;
    m_soundCooldown = delay;
}
void CUnitBody::PlayWoundSound(float cooldown)
{
    if (m_soundCooldown > 0) return;
    int delay = (int)cooldown;
    if (m_visualType->PlayWoundSound() == 160) delay = 0;
    m_soundCooldown = delay;
}
void CUnitBody::ShowBulletMuzzleFlash()
{
    if (m_muzzleFlash != NULL) { m_muzzleFlash->setCurrentController(0, false); m_muzzleFlash->animate(); }
}
void CUnitBody::UnbindFromWorld()
{
    Group world(Game()->m_cameraNode.find(DGHelper::getSwerveID("worldTransform")));
    world.removeChild(m_model);
}
void CUnitBody::AddLightingController(Group group)
{
    if (!CDH_Animation::HaveController(group, "lighting")) return;
    int index = 0;
    while (index <= 4 && m_lighting[index] != NULL) ++index;
    if (index == 5) index = 0; // Original overwrites slot zero when full.
    m_lighting[index] = new CDH_Animation(group);
    m_lighting[index]->addController(XString("lighting"));
    m_lighting[index]->setCurrentController(0, false);
    m_lighting[index]->setRelativePositionAndStop(WindowApp::m_instance->GetRandomValue().Float() * 0.5f);
}
void CUnitBody::BindWeaponToHand()
{
    for (unsigned int lod = 0; lod < m_lodCount; ++lod) {
        if (m_weapons[lod].m_interface == NULL) continue;
        XString name = XString::Format(L"weapon_lod%dTransform", lod + 1);
        Group parent(m_model.find(DGHelper::getSwerveID(XString::AnsiString(name.c_str()))));
        if (parent.m_interface != NULL && m_weapons[lod].m_interface != NULL) parent.addChild(m_weapons[lod]);
    }
}
void CUnitBody::RemoveLightingController(Group group)
{
    for (int index = 0; index <= 4; ++index) {
        if (m_lighting[index] != NULL) {
            bool equal;
            {
                Object3D object = m_lighting[index]->m_object;
                equal = object == group;
            }
            if (equal) {
                if (m_lighting[index] != NULL) { delete m_lighting[index]; m_lighting[index] = NULL; }
                break;
            }
        }
    }
}
void CUnitBody::BlowWeapon()
{
    if (m_weapons[0].m_interface != NULL) {
        Transform transform(CSwerve::GetInstance()->m_swerve);
        m_weapons[0].getTransformTo(Game()->m_cameraNode, transform);
        float matrix[16];
        transform.get(matrix);
        vec3 position(matrix[3], matrix[7], matrix[11]);
        vec3 normal(0.0f, 0.0f, 1.0f);
        Game()->GetDGWorld()->stoneBlast(position, normal);
    }
    RemoveWeapon();
}
void CUnitBody::InitLODChannel(unsigned int lod, Group group)
{
    int channel = m_animatable->getChannel(Object3D(group));
    if (channel != -1) m_lodChannels[lod].AddAll(m_lodChannels[lod].Size(), &channel, 1);
    int count = group.getChildCount();
    for (int index = 0; index < count; ++index) {
        Node child = group.getChild(index);
        if (Group(child).m_interface != NULL) InitLODChannel(lod, Group(child));
        else if (SkinnedMesh(child).m_interface != NULL) InitLODChannel(lod, SkinnedMesh(child));
    }
}
void CUnitBody::InitLODChannel(unsigned int lod, SkinnedMesh mesh)
{
    int channel = m_animatable->getChannel(Object3D(mesh));
    if (channel != -1) m_lodChannels[lod].AddAll(m_lodChannels[lod].Size(), &channel, 1);
    InitLODChannel(lod, mesh.getSkeleton());
}

void CUnitBody::BindBoxToHand(XString name)
{
    if (m_box.m_interface != NULL) { m_box.setRenderingEnable(false); return; }
    bool failed = true;
    {
        Object3D model = App::LoadObject3D("BIN_M3G_OBJECT_CARDBOX");
        if (model.m_interface == NULL) return;
        XString groupName;
        if (name.Length() == 0) groupName = ((App*)WindowApp::m_instance)->m_commonGameConfig->GetRandomBoxGroupName();
        else groupName = ((App*)WindowApp::m_instance)->m_commonGameConfig->GetBoxGroupName(name);
        {
            XString::AnsiString ansi(groupName.c_str());
            m_box = Group(model.find(CSwerve::GenerateUserID(ansi)).duplicate());
        }
        if (m_box.m_interface != NULL) {
            swerve::Image2D<CSwerveAssertingErrorHandler> lightmap = Game()->GetLightmap();
            DGHelper::ChangeImageRecursively(m_box, lightmap, BOXES_LIGHT_TEXTURE_NODE_NAME);
            AddLightingController(m_box);
            Group parent(m_model.find(CSwerve::GenerateUserID("boxTransform")));
            if (parent.m_interface != NULL) {
                if (m_box.m_interface != NULL) parent.addChild(m_box);
                failed = false;
            }
        }
    } // Both the selected XString and the model resource die before the hide call.
    if (!failed && m_box.m_interface != NULL) m_box.setRenderingEnable(false);
}
void CUnitBody::ShowBox(bool show) { if (m_box.m_interface != NULL) m_box.setRenderingEnable(show); }
bool CUnitBody::HasBox() const { return m_box.m_interface != NULL && m_box.getRenderingEnable(); }

bool CalculateRotation(float& yaw, float rate, float elapsed, const vec3& direction)
{
    float target = MathLib::ArcTan(direction.m_x, direction.m_y);
    float oldYaw = yaw;
    float step = rate * elapsed;
    float tolerance = step + 5.0f;
    float normalizedTarget = NormalizeRotation(target);
    float normalizedOld = NormalizeRotation(oldYaw);
    float difference = normalizedOld - normalizedTarget;
    if (difference > 180.0f) difference += -360.0f;
    else if (difference < -180.0f) difference += 360.0f;
    if (difference > tolerance) {
        yaw = NormalizeRotation(yaw - step);
        return false;
    }
    if (difference < -tolerance) {
        yaw = NormalizeRotation(yaw + step);
        return false;
    }
    yaw = target;
    return true;
}

float CUnitBody::GetMovementSpeed()
{
    if (m_owner->m_carrying) return m_visualType->m_carrySpeed * m_scale;
    if (m_owner->m_enraged) return (m_visualType->m_runSpeed + m_visualType->m_runSpeed) * m_scale;
    if (m_owner->m_running) return m_visualType->m_runSpeed * m_scale;
    return m_visualType->m_walkSpeed * m_scale;
}

bool CUnitBody::PickBounds(const CCollision::Line& line)
{
    CCollision::AABB bounds = m_bounds;
    bounds.m_min = bounds.m_min * 1.25f;
    bounds.m_max = bounds.m_max * 1.25f;
    bounds.m_min = bounds.m_min + m_position;
    bounds.m_max = bounds.m_max + m_position;
    vec3 point;
    return CCollision::Intersect(line, bounds, point);
}

bool CUnitBody::Pick(const CCollision::Line& line, vec3& point, vec3& normal)
{
    unsigned char wasPicking = m_model.getPickingEnable();
    m_model.setPickingEnable(true);
    vec3 origin = line.m_from;
    vec3 direction = line.m_to - line.m_from;
    unsigned char hit;
    if (m_lodCount != 0) {
        m_lods[m_currentLod].getTransformTo(Game()->m_cameraNode, m_scratch);
        m_scratch.invert();
        float transformedOrigin[4] = {origin.m_x, origin.m_y, origin.m_z, 1.0f};
        float transformedDirection[4] = {direction.m_x, direction.m_y, direction.m_z, 0.0f};
        m_scratch.transform(4, transformedOrigin);
        m_scratch.transform(4, transformedDirection);
        origin = vec3(transformedOrigin[0], transformedOrigin[1], transformedOrigin[2]);
        direction = vec3(transformedDirection[0], transformedDirection[1], transformedDirection[2]);
        hit = m_lods[m_currentLod].pick(-1, &origin.m_x, &direction.m_x, m_intersection);
    } else {
        m_model.getTransformTo(Game()->m_cameraNode, m_scratch);
        m_scratch.invert();
        float transformedOrigin[4] = {origin.m_x, origin.m_y, origin.m_z, 1.0f};
        float transformedDirection[4] = {direction.m_x, direction.m_y, direction.m_z, 0.0f};
        m_scratch.transform(4, transformedOrigin);
        m_scratch.transform(4, transformedDirection);
        origin = vec3(transformedOrigin[0], transformedOrigin[1], transformedOrigin[2]);
        direction = vec3(transformedDirection[0], transformedDirection[1], transformedDirection[2]);
        hit = m_model.pick(-1, &origin.m_x, &direction.m_x, m_intersection);
    }
    if (hit) {
        float ray[6];
        m_intersection.getRay(ray);
        float length = Length(vec3(ray[3], ray[4], ray[5]));
        m_pickDistance = (m_intersection.getDistance() * length) * m_scale;
        m_intersection.getRay(&m_hitRayOrigin.m_x);
        float distance = m_intersection.getDistance();
        point = line.m_from + (line.m_to - line.m_from) * distance;
        float x = m_intersection.getNormalX();
        float y = m_intersection.getNormalY();
        float z = m_intersection.getNormalZ();
        normal = vec3(x, y, z);
    }
    m_model.setPickingEnable(wasPicking != 0);
    return hit != 0;
}

bool CUnitBody::Pick(float x, float y, float limit, swerve::Camera<CSwerveAssertingErrorHandler>& camera, EHitType& hitType)
{
    unsigned char wasPicking = m_model.getPickingEnable();
    m_model.setPickingEnable(true);
    float screenWidth = (float)((App*)WindowApp::m_instance)->m_helper->m_screenWidth;
    float screenHeight = (float)((App*)WindowApp::m_instance)->m_helper->m_screenHeight;
    swerve::RayIntersection<CSwerveAssertingErrorHandler> intersection(CSwerve::GetInstance()->m_swerve);
    unsigned char hit;
    if (m_lodCount != 0) hit = m_lods[m_currentLod].pick(-1, x / screenWidth, y / screenHeight, camera, intersection);
    else hit = m_model.pick(-1, x / screenWidth, y / screenHeight, camera, intersection);
    if (hit) {
        float ray[6];
        intersection.getRay(ray);
        float length = Length(vec3(ray[3], ray[4], ray[5]));
        m_pickDistance = (intersection.getDistance() * length) * m_scale;
        // Original early failure skips restoration of the picking flag.
        if (limit > 0.0f && m_pickDistance > limit) return false;
        intersection.getRay(&m_hitRayOrigin.m_x);
        float nx = intersection.getNormalX();
        float ny = intersection.getNormalY();
        float nz = intersection.getNormalZ();
        m_hitNormal = vec3(nx, ny, nz);
        m_hitRayFraction = intersection.getDistance();
        hitType = HIT_BODY;
        int userId = intersection.getIntersected().getUserID();
        if (userId == CVisualCharacterType::GetHitzoneMeshUserId()) hitType = HIT_HEAD;
        int specialId = m_visualType->m_specialHitZoneIds[m_currentLod];
        if (specialId != 0 && specialId == userId) hitType = HIT_SPECIAL;
        if (userId == CEnemyWeaponType::GetWeaponHitZoneUserId()) hitType = HIT_WEAPON;
    }
    m_model.setPickingEnable(wasPicking != 0);
    return hit != 0;
}

void CUnitBody::eventListener(const AnimationEventCK& event, WeakIntrusivePtr<AnimationInstance> instance)
{
    if (event.m_id == 0x7ffffffe) m_owner->m_mind.OnAnimationComplete(instance, true);
}
vec3 CUnitBody::GetHitPosition()
{
    vec3 point = m_hitRayOrigin + m_hitRayDirection * m_hitRayFraction;
    m_lods[m_currentLod].getTransformTo(Game()->m_cameraNode, m_scratch);
    SwerveHelper::TransformPoint(&point, &point, m_scratch);
    return point;
}
float CUnitBody::GetDistanceToUnit(CUnit* unit) const { return Length(m_position - unit->m_body.m_position); }
float CUnitBody::GetDistanceToTarget(const CUnitTarget* target) const { return Length(m_position - target->m_position); }
float CUnitBody::GetDistanceToPoint(const vec3& point) const { return Length(m_position - point); }
EPersonAnimType CUnitBody::GetMovementAnimation()
{
    if (m_owner->m_carrying) return (EPersonAnimType)20;
    if (m_owner->m_enraged) return (EPersonAnimType)30;
    if (m_owner->m_running) {
        if (m_weaponShown) return (EPersonAnimType)5;
        return (EPersonAnimType)3;
    }
    return (EPersonAnimType)(2 + (2 ^ ((m_seed << 1) & 2)));
}
EPersonAnimType CUnitBody::GetPainAnimation()
{
    // ?: needed: original sets 11 first then movne 26 (if/else gives the inverted moveq form).
    EPersonAnimType animation = m_owner->m_carrying ? (EPersonAnimType)26 : (EPersonAnimType)11;
    return animation;
}
void CUnitBody::SetLuminance(float value)
{
    float luminance = 0.5f;
    if (value >= 0.0f && value <= 1.0f) luminance = value;
    m_luminance = value;
    float position = 1.0f - luminance;
    for (int index = 0; index < 5; ++index) if (m_lighting[index] != NULL) m_lighting[index]->setRelativePositionAndStop(position);
}

void CUnitBody::GetScreenPos(int& x, int& y, float)
{
    Transform transform(CSwerve::GetInstance()->m_swerve);
    swerve::Camera<CSwerveAssertingErrorHandler> camera = Game()->m_cameraNode.getActiveCamera();
    vec3 direction = UnitVector(Game()->GetCurrentCameraPos() - m_position);
    vec3 view = UnitVector(Game()->m_camera->GetViewDirection());
    float dot = view.Dot(direction);
    if (m_model.getTransformTo(camera, transform)) {
        vec3 height(0.0f, 0.0f, m_height / m_scale);
        transform.postTranslate(&height.m_x);
        static const float vector[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        float projected[4];
        memcpy(projected, vector, sizeof(projected));
        transform.transform(4, projected);
        camera.getProjection(transform);
        transform.transform(4, projected);
        float screenX = (projected[0] / projected[3] + 1.0f) * 0.5f;
        float screenY = (1.0f - projected[1] / projected[3]) * 0.5f;
        int width = WindowApp::m_instance->m_screenWidth;
        int heightPixels = WindowApp::m_instance->m_screenHeight;
        x = (int)(screenX * (float)width);
        y = (int)(screenY * (float)heightPixels);
        if (dot < 0.0f) {
            x = -x;
            y = -y;
            if (x > width / 2) x = width * 2;
            else x = -width;
            if (y > heightPixels / 2) y = heightPixels * 2;
            else y = -heightPixels;
        }
    } else {
        x = 0;
        y = 0;
    }
}
void CUnitBody::SetSameOrientation(const CUnitBody* other, bool reverse)
{
    float yaw = other->m_yaw;
    if (reverse) yaw += 180.0f;
    if (yaw > 180.0f) yaw += -360.0f;
    // original 0x107a9e comparison and addition share +360.0f; keeps the original threshold.
    if (yaw < 360.0f) yaw += 360.0f;
    m_yaw = yaw;
    m_transformDirty = true;
    m_orientationControl = ORIENTATION_NONE;
}
void CUnitBody::SetState(UnitState state) { m_state = state; }

bool CUnitBody::Intersect(const CCollision::Line& line, vec3& point, vec3& normal)
{
    CCollision::AABB box;
    box.m_min = m_bounds.m_min + m_position;
    box.m_max = m_bounds.m_max + m_position;
    bool hit = CCollision::Intersect(line, box, point);
    if (!hit) return hit;
    normal = UnitVector(line.m_from - line.m_to);
    if (Ballistics::g_CurrentlySimulatedBallistics == Game()->GetCollisionBallistics()) return hit;
    int flags = 1;
    if (Fragments::g_CurrentlySimulatedFragments != NULL) flags = Fragments::GetFlag(Ballistics::g_CurrentlyCollidingProjectile);
    const CGrenade* grenade = ((App*)WindowApp::m_instance)->m_grenadeManager->GetActiveGrenade();
    float multiplier = 0.0f;
    if (grenade != NULL) multiplier = grenade->m_shardMulti;
    bool weaponDamage = (flags & 2) != 0;
    if (weaponDamage) multiplier = 1.0f;
    float frameTime = WindowApp::m_instance->m_frameTimeSeconds;
    if (((App*)WindowApp::m_instance)->m_powerUpManager->IsPowerUpRunning()) frameTime *= ((App*)WindowApp::m_instance)->m_powerUpManager->GetActivePowerUp()->m_reloadSoundSpeed;
    float distance = Length(line.m_from - line.m_to);
    int damage;
    if (!weaponDamage) {
        damage = (int)((distance / frameTime) * multiplier);
    } else {
        const CDH_Weapon* weapon = ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
        int value = CRandGen::GetInstance()->GetRandRange(weapon->m_damageMin, weapon->m_damageMax);
        switch (weapon->m_weaponType) {
        case 1: case 7: value /= 7; break;
        case 8: value /= 2; break;
        case 9: value /= 3; break;
        }
        damage = value / 2;
    }
    if (damage < 2) return hit;
    const CDH_Weapon* weapon = NULL;
    if (weaponDamage) {
        weapon = ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
        CBH_Player::GetInstance()->m_gameStats.NotifyShotBegin(weapon, NULL, 1);
    } else {
        CBH_Player::GetInstance()->m_gameStats.NotifyShotBegin(NULL, grenade, 1);
    }
    // The original reacquires the active weapon after NotifyShotBegin.
    if (weaponDamage) weapon = ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
    if ((flags & 1) == 0) grenade = NULL;
    m_owner->ReceivedFragDamage(damage, weapon, grenade);
    CBH_Player::GetInstance()->m_gameStats.NotifyShotEnd();
    return hit;
}



