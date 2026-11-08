// CLocation.cpp - read the location description and its scene bindings.
#include "CLocation.h"
#include "CXmlHelper.h"
#include "App.h"
#include "ResMap.h"
#include "WindowBase.h"
#include "CSceneObjectManager.h"

CLocation::CLocation(TiXmlNode* node)
{
    Parse(node);
}

void CLocation::Parse(TiXmlNode* node)
{
    m_name = CXmlHelper::GetAttributeValue(node, "name");
    ParseInfo(node->FirstChild("info"));
    ParseModel(node->FirstChild("model"));
    ParseAmbient(node->FirstChild("ambient"));
    ParseGraphics(node->FirstChild("graphics")->ToElement());
    ParseObjects(node->FirstChild("objects"));
    ParseSectors(node);
    ParseOptionalParts(node->FirstChild("optional_parts"));
    TiXmlNode* lighting = node->FirstChild("lighting");
    if (lighting != NULL) {
        m_lightingNode = CXmlHelper::GetAttributeValue(lighting, "node");
    }
}

void CLocation::ParseInfo(TiXmlNode* node)
{
    if (node != NULL) {
        m_title = Window::ResStringSafe(CXmlHelper::GetAttributeValue(node, "location_title"));
        m_cameraName = CXmlHelper::GetAttributeValueOrUseDefault(node, "camera", "Camera01");
        m_cameraMaxAngleH = CXmlHelper::GetAttributeValue(node, "camera_max_angle_h").ToFloat();
        m_cameraMaxAngleV = CXmlHelper::GetAttributeValue(node, "camera_max_angle_v").ToFloat();
        m_baseLuminance = CXmlHelper::GetAttributeValueOrUseDefault(node, "base_scene_luminance", "1.0").ToFloat();
        m_sectorCapacity = CXmlHelper::GetAttributeValueOrUseDefault(node, "octree_sector_capacity", "32").ToInt();
    }
}

void CLocation::ParseModel(TiXmlNode* node)
{
    if (node != NULL) {
        m_bigModel = CXmlHelper::GetAttributeValue(node, "m3g_big");
        m_smallModel = CXmlHelper::GetAttributeValue(node, "m3g_small");
        m_light = CXmlHelper::GetAttributeValue(node, "light");
    }
}

void CLocation::ParseAmbient(TiXmlNode* node)
{
    if (node != NULL) {
        if (node->ToElement()->Attribute("wav") != NULL) {
            XString resource = CXmlHelper::GetAttributeValue(node, "wav");
            m_ambientResource = ((App*)WindowApp::m_instance)->m_resMap->getResourceID(resource);
        } else if (node->ToElement()->Attribute("ogg") != NULL) {
            XString resource = CXmlHelper::GetAttributeValue(node, "ogg");
            m_ambientResource = ((App*)WindowApp::m_instance)->m_resMap->getResourceID(resource);
        }
    }
}

void CLocation::ParseGraphics(TiXmlElement* element)
{
    if (element != NULL) {
        m_cameraImagesKeyset = CXmlHelper::GetAttributeValue(element, "camera_images_keyset");
    }
}

void CLocation::ParseObjects(TiXmlNode* node)
{
    if (node == NULL) {
        return;
    }
    for (TiXmlNode* child = node->FirstChild("object"); child != NULL; child = child->NextSibling("object")) {
        SObject object;
        object.m_name = CXmlHelper::GetAttributeValue(child, "name");
        object.m_pointName = CXmlHelper::GetAttributeValue(child, "pointName");
        ((App*)WindowApp::m_instance)->m_sceneObjectManager->GetSceneObjectInfoByName(object.m_name);
        m_objects.addElement(object);
    }
}

void CLocation::ParseSectors(TiXmlNode* node)
{
    if (node == NULL) {
        return;
    }
    for (TiXmlNode* child = node->FirstChild("sector"); child != NULL; child = child->NextSibling("sector")) {
        SSector sector;
        sector.m_name = CXmlHelper::GetAttributeValue(child, "name");
        for (TiXmlNode* point = child->FirstChild("point"); point != NULL; point = point->NextSibling("point")) {
            sector.m_points.addElement(CXmlHelper::GetAttributeValue(point, "name"));
        }
        m_sectors.addElement(sector);
    }
}

void CLocation::ParseOptionalParts(TiXmlNode* node)
{
    if (node == NULL) {
        return;
    }
    for (TiXmlNode* child = node->FirstChild("part"); child != NULL; child = child->NextSibling("part")) {
        SOptionalPart part;
        part.m_name = CXmlHelper::GetAttributeValue(child, "name");
        part.m_group = CXmlHelper::GetAttributeValue(child, "group");
        part.m_hidden = CXmlHelper::GetAttributeValueOrUseDefault(child, "hidden", "0").ToBool();
        m_parts.addElement(part);
    }
}
