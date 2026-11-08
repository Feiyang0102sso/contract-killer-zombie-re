// CSceneObject.cpp - original u441 scene object model duplication, binding, picking and lighting control.
#include "CSceneObject.h"
#include "CGameAIMap.h"
#include "CSwerveGame.h"
#include "App.h"
#include "DGHelper.h"
#include "CDH_Animation.h"
#include "SwerveHelper.h"
#include "CSwerve.h"

typedef swerve::Group<CSwerveAssertingErrorHandler> Group;
typedef swerve::Object3D<CSwerveAssertingErrorHandler> Object3D;
typedef swerve::QueryInterface<CSwerveAssertingErrorHandler> QueryInterface;
typedef swerve::Transform<CSwerveAssertingErrorHandler> Transform;
typedef swerve::Image2D<CSwerveAssertingErrorHandler> Image2D;

CSceneObject::CSceneObject(const CSceneObjectManager::SSceneObjectInfo* info, bool pickingEnabled)
    : m_lightAnimation(NULL)
{
    if (info->m_model.m_interface == NULL) {
        static_cast<QueryInterface&>(info->m_model) = App::LoadObject3D(info->m_modelResource).m_interface;
    }
    Group object(info->m_model.find(SwerveHelper::Id(XString::AnsiString(info->m_objectTransform.m_data))));
    // The original first duplicates, then processes the copy via FullObjectDuplicate; keeps both copy levels and the temporary reference.
    m_group = SwerveHelper::FullObjectDuplicate(Group(object.duplicate()), 0);
    m_group.setPickingEnable(false);
    m_mesh = Group(m_group.find(SwerveHelper::Id(XString::AnsiString(info->m_meshTransform.m_data))));
    if (m_mesh.m_interface != NULL) {
        m_mesh.setPickingEnable(pickingEnabled);
    }
    if (info->m_lightNodeName.ByteSize() != 0) {
        const Image2D& lightmap = ((App*)WindowApp::m_instance)->m_helper->m_swerveGame->GetLightmap();
        if (lightmap.m_interface != NULL) {
            Image2D image = lightmap;
            DGHelper::ChangeImageRecursively(m_group, image, XString::AnsiString(info->m_lightNodeName.m_data));
        }
    }
    if (CDH_Animation::HaveController(m_group, "lighting") == true) {
        m_lightAnimation = new CDH_Animation(m_group);
        m_lightAnimation->addController(XString("lighting"));
        m_lightAnimation->setCurrentController(0, false);
        m_lightAnimation->setRelativePositionAndStop(0.0f);
    }
    BindToWorld();
}

void CSceneObject::BindToWorld()
{
    CSwerveGame* game = ((App*)WindowApp::m_instance)->m_helper->m_swerveGame;
    Group parent(game->m_cameraNode.find(DGHelper::getSwerveID("worldTransform")));
    parent.addChild(m_group);
}

void CSceneObject::SetToPoint(const IGameAIMap* map, XString point)
{
    int id = map->GetPointIDByName(point);
    m_pointName = point;
    SetToPoint(map, id);
}

void CSceneObject::SetToPoint(const IGameAIMap* map, int id)
{
    const vec3& position = map->GetMapPointPostition(id);
    const float* orientation = map->GetMapPointOrientation(id);
    m_position = position;
    Transform transform(CSwerve::GetInstance()->m_swerve);
    float translation[3] = { m_position.m_x, m_position.m_y, m_position.m_z };
    transform.postTranslate(translation);
    transform.postRotate(orientation);
    m_group.setTransform(transform);
}

CSceneObject::~CSceneObject()
{
    if (m_lightAnimation != NULL) {
        delete m_lightAnimation;
        m_lightAnimation = NULL;
    }
}

bool CSceneObject::Pick(float x, float y, float distance, swerve::Camera<CSwerveAssertingErrorHandler>& camera)
{
    if (CanBeHandled() != true) {
        return false;
    }
    int width = ((App*)WindowApp::m_instance)->m_helper->m_screenWidth;
    int height = ((App*)WindowApp::m_instance)->m_helper->m_screenHeight;
    swerve::RayIntersection<CSwerveAssertingErrorHandler> intersection(CSwerve::GetInstance()->m_swerve);
    if (m_mesh.m_interface == NULL || !m_mesh.pick(-1, x / (float)width, y / (float)height, camera, intersection)) {
        return false;
    }
    float hitDistance = intersection.getDistance();
    if (distance < 0.0f) {
        return true;
    }
    return hitDistance < distance;
}

void CSceneObject::SetLuminance(float luminance)
{
    if (m_lightAnimation == NULL) {
        return;
    }
    float position = 0.5f;
    if (luminance >= 0.0f && luminance <= 1.0f) {
        position = luminance;
    }
    m_lightAnimation->setRelativePositionAndStop(1.0f - position);
}
