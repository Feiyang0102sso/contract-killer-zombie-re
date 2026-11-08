// DGHelper.cpp - helpers of the game: Swerve utilities, drawing, string formatting.
#include "DGHelper.h"
#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#include "CSwerve.h"
#include "CApplet.h"
#include "CApp.h"
#include "ICCore.h"
#include "CHash.h"
#include "CRandGen.h"
#include "CVector.h"
#include "CStdUtil_Cocoa.h"
#include "ICGraphics.h"
#include "ICGraphics2d.h"
#include "ICRenderSurface.h"
#include "CFont.h"
#include "CColor.h"
#include "CResource.h"
#include "ICResourceManager.h"
#include "CDrawUtil.h"
#include "DataInputStream.h"
#include "MathLib.h"
#include "App.h"
#include "CDH_ResourceManager.h"
#include "AnimationImpl.h"
#include "AnimatableSwerveObject.h"
#include "timelib.h"

using namespace com::glu::platform;

/** Rating page of the game. */
static const wchar_t RATE_ME_LINK[] =
    L"http://gcs.glu.com/gcs/iphone/iphone.jsp?t=zombsniper&src=zombsniper&p=iphone&page=review";

/** DrawImageScaled: alpha below this is not drawn, above the next one needs no blending. */
static const float ALPHA_INVISIBLE = 0.01f;
static const float ALPHA_OPAQUE = 0.99f;

/** Swerve interface id of World (the aux render scene). */
static const int SWERVE_WORLD = 0x1f;

int DGHelper::m_alpha_blend_stack_pos;

/** Count one alpha blended draw (only alpha within 0..1 counts). */
static inline void PushAlphaBlend(float alpha)
{
    if (alpha >= 0.0f && alpha <= 1.0f) {
        graphics::ICGraphics2d::GetInstance();
        DGHelper::m_alpha_blend_stack_pos++;
    }
}

/** Undo PushAlphaBlend. */
static inline void PopAlphaBlend()
{
    if (DGHelper::m_alpha_blend_stack_pos >= 1) {
        DGHelper::m_alpha_blend_stack_pos--;
    }
}

DGHelper::DGHelper()
    : m_transform(CSwerve::GetInstance()->m_swerve),
      m_rayIntersection(CSwerve::GetInstance()->m_swerve),
      m_swerveGame(NULL),
      m_gpsScreen(NULL),
      m_screenWidth(0),
      m_screenHeight(0),
      m_field118(0),
      m_field11c(0),
      m_startTimeMs(0),
      m_config(L"res/common/configs/dh3d_config.cfg"),
      m_loadingResource(false)
{
    m_startTimeMs = CStdUtil_Cocoa::GetTimeSeconds() * 1000;
    m_swerveGame = NULL;
    m_gpsScreen = NULL;
    m_gamePlayManager.init();
    initGame();
    if (m_auxRender == NULL) {
        swerve::Object3D<CSwerveAssertingErrorHandler> root(
            (IQueryInterface*)CSwerve::GetInstance()->LoadM3GResource("BIN_AUX_RENDER"));
        m_auxRender = swerve::World<CSwerveAssertingErrorHandler>(root);
    }
}

void DGHelper::initGame()
{
    graphics::ICRenderSurface* screen = graphics::ICGraphics::GetInstance()->GetTargetSurface();
    unsigned int height;
    unsigned int width;
    screen->GetWidthAndHeight(width, height);
    m_screenWidth = width;
    m_screenHeight = height;
}

void DGHelper::initSwerveGame(CSwerveGame* game)
{
    m_swerveGame = game;
    initGame();
}

void DGHelper::SetSwerveGamePtr(CSwerveGame* game)
{
    m_swerveGame = game;
}

void DGHelper::initGPSScreen(CGPSMapScreen* screen)
{
    m_gpsScreen = screen;
    initGame();
}

DGHelper::~DGHelper()
{
}

int DGHelper::getSwerveID(const char* name)
{
    return CSwerve::GenerateUserID(name);
}

void DGHelper::SetCameraSize(swerve::Camera<CSwerveAssertingErrorHandler>& camera, int width, int height,
                             camera_size_update_type_t updateType)
{
    float params[4];
    camera.getProjection(params);
    float aspect;
    switch (updateType) {
    case CAMERA_SIZE_SET_ASPECT:
    case CAMERA_SIZE_SET_ASPECT_ONLY:
        aspect = (float)width / (float)height;
        params[1] = aspect;
        break;
    case CAMERA_SIZE_KEEP_FOV: {
        aspect = (float)width / (float)height;
        float oldAspect = params[1];
        params[1] = aspect;
        params[0] = params[0] * (oldAspect / aspect);
        break;
    }
    default:
        aspect = params[1];
        break;
    }
    camera.setPerspective(params[0], aspect, params[2], params[3]);
}

void DGHelper::TransformPoint(swerve::Transform<CSwerveAssertingErrorHandler>& transform, vec3& point)
{
    float vector[4];
    vector[0] = point.m_x;
    vector[1] = point.m_y;
    vector[2] = point.m_z;
    vector[3] = 1.0f;
    transform.transform(4, vector);
    point.m_x = vector[0];
    point.m_y = vector[1];
    point.m_z = vector[2];
}

void DGHelper::TransformPoint(swerve::Transform<CSwerveAssertingErrorHandler>& transform, swvVector3D& point)
{
    // swvVector3D has the layout of vec3 (x, y, z floats)
    TransformPoint(transform, (vec3&)point);
}

void DGHelper::DrawCircle(int centerX, int centerY, int radius, int color, bool filled)
{
    graphics::ICGraphics2d* graphics = graphics::ICGraphics2d::GetInstance();
    int x = 0;
    int y = radius;
    DrawCirclePixels(centerX, centerY, x, y, color, graphics, filled);
    int decision = 1 - radius;
    int deltaEast = 3;
    int deltaSouthEast = 5 - 2 * radius;
    while (y > x) {
        if (decision < 0) {
            decision += deltaEast;
        } else {
            decision += deltaSouthEast;
            deltaSouthEast += 2;
            y--;
        }
        deltaEast += 2;
        deltaSouthEast += 2;
        x++;
        DrawCirclePixels(centerX, centerY, x, y, color, graphics, filled);
    }
}

void DGHelper::DrawCirclePixels(int centerX, int centerY, int x, int y, int color,
                                graphics::ICGraphics2d* graphics, bool filled)
{
    for (int pass = 0; pass < 2; pass++) {
        if (filled) {
            CDrawUtil::Fill(centerX - x, centerY - y, centerX + x + 1, centerY - y + 1, color,
                            (graphics::ICGraphics2d::Primitive)3);
            CDrawUtil::Fill(centerX - x, centerY + y, centerX + x + 1, centerY + y + 1, color,
                            (graphics::ICGraphics2d::Primitive)3);
        } else {
            CDrawUtil::Fill(centerX - x, centerY - y, centerX - x + 1, centerY - y + 1, color,
                            (graphics::ICGraphics2d::Primitive)3);
            CDrawUtil::Fill(centerX - x, centerY + y, centerX - x + 1, centerY + y + 1, color,
                            (graphics::ICGraphics2d::Primitive)3);
            CDrawUtil::Fill(centerX + x, centerY + y, centerX + x + 1, centerY + y + 1, color,
                            (graphics::ICGraphics2d::Primitive)3);
            CDrawUtil::Fill(centerX + x, centerY - y, centerX + x + 1, centerY - y + 1, color,
                            (graphics::ICGraphics2d::Primitive)3);
        }
        // the second pass draws the octants mirrored at the diagonal
        int swap = x;
        x = y;
        y = swap;
    }
}

void DGHelper::DrawImageScaled(graphics::ICRenderSurface* surface, float scaleX, float scaleY, int x, int y,
                               float alpha, int pivotX, int pivotY, float rotation, unsigned int flags)
{
    graphics::ICGraphics2d* graphics = graphics::ICGraphics2d::GetInstance();
    if (alpha < ALPHA_INVISIBLE) {
        return;
    }
    if (flags != 0) {
        unsigned int width;
        unsigned int height;
        surface->GetWidthAndHeight(width, height);
        if (flags & DRAW_RIGHT) {
            pivotX += (int)(width * scaleX);
        } else if (flags & DRAW_HCENTER) {
            pivotX += (int)(width * scaleX) >> 1;
        }
        if (flags & DRAW_BOTTOM) {
            pivotY += (int)(height * scaleY);
        } else if (flags & DRAW_VCENTER) {
            pivotY += (int)(height * scaleY) >> 1;
        }
    }
    if (alpha < ALPHA_OPAQUE) {
        PushAlphaBlend(alpha);
    }
    graphics->PushTransform();
    graphics->LoadIdentity();
    graphics->Translate((float)x, (float)y);
    if (rotation != 0) {
        graphics->Rotate(rotation);
    }
    if (pivotX != 0 || pivotY != 0) {
        graphics->Translate((float)-pivotX, (float)-pivotY);
    }
    if (scaleX != 1.0f || scaleY != 1.0f) {
        graphics->Scale(scaleX, scaleY);
    }
    components::Color_RGBA color;
    color.r = 1.0f;
    color.g = 1.0f;
    color.b = 1.0f;
    color.a = alpha;
    graphics->Draw(*surface, NULL, &color, (graphics::ICGraphics2d::Flip)0);
    graphics->PopTransform();
    if (alpha < ALPHA_OPAQUE) {
        PopAlphaBlend();
    }
}

void DGHelper::DrawImage(graphics::ICRenderSurface* surface, int x, int y, float alpha, int pivotX, int pivotY,
                         float rotation, unsigned int flags)
{
    DrawImageScaled(surface, 1.0f, 1.0f, x, y, alpha, pivotX, pivotY, rotation, flags);
}

bool DGHelper::LoadAnims(Vector<AnimationImpl*>& animations, const char* name)
{
    systems::CResource* resource = NULL;
    systems::ICResourceManager* resources = (systems::ICResourceManager*)CApplet::m_pApp->m_resourceManager;
    resources->CreateResource(name, &resource);
    if (resource == NULL) {
        return false;
    }
    {
        JDataInputStream stream((components::CBinary*)resource->GetData());
        AnimationImpl::load(&stream, animations);
        ((systems::ICResourceManager*)CApplet::m_pApp->m_resourceManager)->ReleaseResource(name);
    }
    return resource != NULL;
}

bool DGHelper::CreateAnimatableObject(swerve::Group<CSwerveAssertingErrorHandler> group,
                                      AnimatableSwerveObject** object, const char* name)
{
    systems::CResource* resource = NULL;
    if (group.m_interface == NULL) {
        return false;
    }
    ((systems::ICResourceManager*)CApplet::m_pApp->m_resourceManager)->CreateResource(name, &resource);
    if (resource == NULL) {
        return false;
    }
    {
        JDataInputStream stream((components::CBinary*)resource->GetData());
        *object = new AnimatableSwerveObject(group, &stream);
        ((systems::ICResourceManager*)CApplet::m_pApp->m_resourceManager)->ReleaseResource(name);
    }
    return resource != NULL;
}

void DGHelper::GetPosition(swerve::Transform<CSwerveAssertingErrorHandler> transform, vec3& position)
{
    position = GetPosition(transform);
}

vec3 DGHelper::GetPosition(swerve::Transform<CSwerveAssertingErrorHandler> transform)
{
    swerve::Transform<CSwerveAssertingErrorHandler> local = transform;
    float matrix[16];
    local.get(matrix);
    vec3 position;
    position.m_x = matrix[3];
    position.m_y = matrix[7];
    position.m_z = matrix[11];
    return position;
}

int DGHelper::GetTextWidth(const char* text, graphics::CFont* font)
{
    int length = strlen(text);
    int width = 0;
    if (length > 0) {
        for (int index = 0; index < length; index++) {
            width += font->GetWidth(text[index]);
        }
        width = font->GetMinCharSpacing() * (length - 1) + width;
    }
    return width;
}

void DGHelper::getUserParameter(swerve::Object3D<CSwerveAssertingErrorHandler>& object, int id, char** value)
{
    if (object.m_interface == NULL) {
        return;
    }
    int length = object.getUserParameter(id, 0, NULL);
    if (length < 1) {
        return;
    }
    *value = new char[length + 1];
    (*value)[length] = 0;
    object.getUserParameter(id, length, *value);
}

void DGHelper::getUserParameter(swerve::Object3D<CSwerveAssertingErrorHandler>& object, int id, float* value)
{
    char* text = NULL;
    getUserParameter(object, id, &text);
    if (text != NULL) {
        *value = atof(text);
        delete[] text;
        text = NULL;
    }
}

void DGHelper::getUserParameter(swerve::Object3D<CSwerveAssertingErrorHandler>& object, const char* name,
                                char** value)
{
    getUserParameter(object, CSwerve::GenerateUserID(name), value);
}

void DGHelper::getUserParameter(swerve::Object3D<CSwerveAssertingErrorHandler>& object, const char* name,
                                float* value)
{
    getUserParameter(object, CSwerve::GenerateUserID(name), value);
}

void DGHelper::getUserParameter(swerve::Object3D<CSwerveAssertingErrorHandler>& object, const char* name,
                                int* value)
{
    float number = -1.0f;
    getUserParameter(object, CSwerve::GenerateUserID(name), &number);
    *value = (int)number;
}

float DGHelper::sqrt(float value)
{
    float half = value * 0.5f;
    int bits = *(int*)&value;
    bits = 0x5f3759df - (bits >> 1);
    float inverse = *(float*)&bits;
    inverse = inverse * (1.5f - (inverse * inverse) * half);
    return value * inverse;
}

game_type_t DGHelper::GetGameType()
{
    return m_gamePlayManager.m_gameType;
}

CDH_PlayerData* DGHelper::GetPlayerData()
{
    return &m_gamePlayManager.m_playerData;
}

CDH_BasicGameData* DGHelper::GetGameData()
{
    return m_gamePlayManager.GetGameData();
}

void DGHelper::LaunchRateMe()
{
    CApplet::m_pCore->LaunchURL(RATE_ME_LINK, 0);
}

XString DGHelper::GetRandomBackgroundResId()
{
    CDH_ResourceManager* resources = ((App*)WindowApp::m_instance)->m_resourceManager;
    const char* keySet;
    if (App::IsXGA()) {
        keySet = "KEYSET_LOADING_SCREENS_XGA";
    } else {
        keySet = "KEYSET_LOADING_SCREENS";
    }
    CDH_KeySet* keys = (CDH_KeySet*)resources->getResourceData(keySet, false);
    core::CRandGen* random = core::CRandGen::GetInstance();
    int index = random->GetRandRange(0, keys->m_count - 1);
    const char* key = keys->GetKey(index);
    XString result(key);
    ((App*)WindowApp::m_instance)->m_resourceManager->releaseResource("KEYSET_LOADING_SCREENS");
    return result;
}

XString DGHelper::ClockStringFromSeconds(unsigned int seconds)
{
    unsigned int hours = seconds / 3600;
    unsigned int rest = seconds - hours * 3600;
    unsigned int minutes = rest / 60;
    unsigned int secondsOfMinute = rest - minutes * 60;
    XString result;
    if (hours > 0) {
        result.Append(XString::Format(L"%d", hours) + L":");
    }
    if (minutes > 0 || hours > 0) {
        result.Append(XString::Format(L"%02d", minutes) + L":");
    } else if (hours == 0) {
        result.Append(L"0:", wcslen(L"0:") * sizeof(wchar_t));
    }
    result.Append(XString::Format(L"%02d", secondsOfMinute));
    return result;
}

int DGHelper::GetDaysFromTimestamp(unsigned int timestamp)
{
    timelib_time time;
    timelib_time2local(&time, 172800);
    return (time.sse + timestamp) / 86400;
}

XString DGHelper::FormatMoneyString(int money)
{
    XString digits(money);
    XString result;
    for (int index = 0; index < digits.Length(); index++) {
        if (index % 3 == 0 && index >= 1) {
            result = XString(",") + result;
        }
        wchar_t character = digits.Char(digits.Length() - 1 - index);
        result.InsertData(0, &character, sizeof(wchar_t));
    }
    return result;
}

void DGHelper::SplitStringAndTrim(Vector<XString>& parts, XString& text, const XString& separator)
{
    text.Split(parts, separator.c_str());
    for (int index = 0; index < parts.size(); index++) {
        parts[index] = parts[index].Trim(true, true);
    }
}

bool DGHelper::CropName(XString& name, int maxLength)
{
    if (maxLength < 4) {
        return false;
    }
    if (name.Length() <= maxLength) {
        return false;
    }
    name = name.SubString(0, maxLength - 3);
    name.Append(XString("..."));
    return true;
}

bool DGHelper::ChangeImageRecursively(swerve::Object3D<CSwerveAssertingErrorHandler> object,
                                      swerve::Image2D<CSwerveAssertingErrorHandler> image, const char* name)
{
    bool changed = false;
    swerve::Mesh<CSwerveAssertingErrorHandler> mesh(object);
    if (mesh.m_interface != NULL) {
        int submeshCount = mesh.getSubmeshCount();
        for (int submesh = 0; submesh < submeshCount; submesh++) {
            swerve::Appearance<CSwerveAssertingErrorHandler> appearance = mesh.getAppearance(submesh);
            if (appearance.m_interface != NULL) {
                for (int unit = 0; unit < 2; unit++) {
                    swerve::Texture2D<CSwerveAssertingErrorHandler> texture = appearance.getTexture(unit);
                    if (texture.m_interface != NULL) {
                        if (name == NULL || texture.getUserID() == CSwerve::GenerateUserID(name)) {
                            changed = true;
                            texture.setImage(image);
                        }
                    }
                }
            }
        }
    }
    swerve::Group<CSwerveAssertingErrorHandler> group(object);
    if (group.m_interface != NULL) {
        int childCount = group.getChildCount();
        for (int index = 0; index < childCount; index++) {
            swerve::Node<CSwerveAssertingErrorHandler> child = group.getChild(index);
            changed |= ChangeImageRecursively(child, image, name);
        }
    }
    return changed;
}

void DGHelper::SetUserIDRecursively(swerve::Object3D<CSwerveAssertingErrorHandler> object, int userId)
{
    swerve::Mesh<CSwerveAssertingErrorHandler> mesh(object);
    if (mesh.m_interface != NULL) {
        mesh.setUserID(userId);
    }
    swerve::Group<CSwerveAssertingErrorHandler> group(object);
    if (group.m_interface != NULL) {
        int childCount = group.getChildCount();
        for (int index = 0; index < childCount; index++) {
            swerve::Node<CSwerveAssertingErrorHandler> child = group.getChild(index);
            SetUserIDRecursively(child, userId);
        }
    }
}

XString DGHelper::GetFriendName(int index, XString name)
{
    XString friendName;
    if (index != -1) {
        friendName = L"Test User Name";
    }
    return XString(L"");
}
