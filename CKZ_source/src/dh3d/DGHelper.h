// DGHelper.h - helpers of the game: Swerve utilities, drawing, string formatting and the game state
// shared by the screens (owned by App, reached through App::m_helper).
#ifndef DGHELPER_H
#define DGHELPER_H

#include <stddef.h>
#include "tws.h"
#include "xString.h"
#include "Vector_ss.h"
#include "DataOutputStream.h"
#include "GamePlayManager.h"
#include "ConfigReader.h"
#include "CMatrix2d.h"

class CSwerveGame;
class CGPSMapScreen;
class AnimationImpl;
class AnimatableSwerveObject;
class vec3;
struct swvVector3D;

namespace com { namespace glu { namespace platform { namespace graphics {
class ICGraphics2d;
class ICRenderSurface;
class CFont;
} } } }

/** How SetCameraSize changes the projection (values from the call sites). */
enum camera_size_update_type_t {
    CAMERA_SIZE_SET_ASPECT = 0,         // aspect ratio = width / height
    CAMERA_SIZE_SET_ASPECT_ONLY = 1,    // same as 0
    CAMERA_SIZE_KEEP_FOV = 2            // new aspect ratio, field of view scaled to keep the old view
};

class DGHelper {
public:
    /** DrawImageScaled flags: anchor of the position (the default is top left). */
    enum {
        DRAW_RIGHT = 4,
        DRAW_HCENTER = 0x10,
        DRAW_BOTTOM = 8,
        DRAW_VCENTER = 0x20
    };

    DGHelper();
    ~DGHelper();

    /** Read the screen size. */
    void initGame();
    void initSwerveGame(CSwerveGame* game);
    void SetSwerveGamePtr(CSwerveGame* game);
    void initGPSScreen(CGPSMapScreen* screen);

    /** M3G user id of `name`. */
    static int getSwerveID(const char* name);
    /** Set the projection of `camera` for a `width` x `height` viewport. */
    void SetCameraSize(swerve::Camera<CSwerveAssertingErrorHandler>& camera, int width, int height,
                       camera_size_update_type_t updateType);
    /** Transform the point `point` in place with `transform`. */
    static void TransformPoint(swerve::Transform<CSwerveAssertingErrorHandler>& transform, vec3& point);
    static void TransformPoint(swerve::Transform<CSwerveAssertingErrorHandler>& transform, swvVector3D& point);

    /** Midpoint circle around (centerX, centerY). */
    void DrawCircle(int centerX, int centerY, int radius, int color, bool filled);
    /** The 8 symmetric pixels (or spans when `filled`) of one circle step. */
    void DrawCirclePixels(int centerX, int centerY, int x, int y, int color,
                          com::glu::platform::graphics::ICGraphics2d* graphics, bool filled);
    /** Draw `surface` scaled, rotated and with `alpha` (0..1); `flags` select the anchor. */
    void DrawImageScaled(com::glu::platform::graphics::ICRenderSurface* surface, float scaleX, float scaleY,
                         int x, int y, float alpha, int pivotX, int pivotY, float rotation, unsigned int flags);
    void DrawImage(com::glu::platform::graphics::ICRenderSurface* surface, int x, int y, float alpha,
                   int pivotX, int pivotY, float rotation, unsigned int flags);

    /** Load the animations of resource `name` into `animations`; false when the resource is missing. */
    bool LoadAnims(Vector<AnimationImpl*>& animations, const char* name);
    /** New animatable object for `group` from resource `name`; false when the resource is missing. */
    bool CreateAnimatableObject(swerve::Group<CSwerveAssertingErrorHandler> group, AnimatableSwerveObject** object,
                                const char* name);
    /** Translation part of `transform`. */
    static void GetPosition(swerve::Transform<CSwerveAssertingErrorHandler> transform, vec3& position);
    static vec3 GetPosition(swerve::Transform<CSwerveAssertingErrorHandler> transform);
    /** Width of `text` in `font` (characters plus spacing). */
    int GetTextWidth(const char* text, com::glu::platform::graphics::CFont* font);

    /** User parameter `id` of `object` as a new string (left unchanged when absent). */
    void getUserParameter(swerve::Object3D<CSwerveAssertingErrorHandler>& object, int id, char** value);
    void getUserParameter(swerve::Object3D<CSwerveAssertingErrorHandler>& object, int id, float* value);
    void getUserParameter(swerve::Object3D<CSwerveAssertingErrorHandler>& object, const char* name, char** value);
    void getUserParameter(swerve::Object3D<CSwerveAssertingErrorHandler>& object, const char* name, float* value);
    void getUserParameter(swerve::Object3D<CSwerveAssertingErrorHandler>& object, const char* name, int* value);

    /** Fast square root (inverse square root approximation, one Newton step). */
    static float sqrt(float value);

    game_type_t GetGameType();
    CDH_PlayerData* GetPlayerData();
    CDH_BasicGameData* GetGameData();

    /** Open the rating page of the game. */
    static void LaunchRateMe();
    /** Resource id of a random loading screen background. */
    XString GetRandomBackgroundResId();
    /** "h:mm:ss" / "m:ss" / "0:ss" text of `seconds`. */
    static XString ClockStringFromSeconds(unsigned int seconds);
    /** Days since the epoch of a unix timestamp (local time). */
    static int GetDaysFromTimestamp(unsigned int timestamp);
    /** `money` with thousands separators. */
    static XString FormatMoneyString(int money);
    /** Split `text` at `separator` into `parts` and trim every part. */
    static void SplitStringAndTrim(Vector<XString>& parts, XString& text, const XString& separator);
    /** Shorten `name` to `maxLength` characters ending in "..."; true when it was shortened. */
    static bool CropName(XString& name, int maxLength);
    /** Put `image` into the textures of `object` and its descendants (only textures with user id `name` when given). */
    static bool ChangeImageRecursively(swerve::Object3D<CSwerveAssertingErrorHandler> object,
                                       swerve::Image2D<CSwerveAssertingErrorHandler> image, const char* name);
    /** Set user id `userId` on the meshes of `object` and its descendants. */
    static void SetUserIDRecursively(swerve::Object3D<CSwerveAssertingErrorHandler> object, int userId);
    /** Display name of friend `index`, `name` when it is not -1. */
    static XString GetFriendName(int index, XString name);

    /** Depth of DrawImageScaled calls that changed the alpha. */
    static int m_alpha_blend_stack_pos;

    int m_field00;                                                  // 0x00 not touched here
    swerve::Transform<CSwerveAssertingErrorHandler> m_transform;    // 0x04 scratch transform
    int m_field08[3];                                               // 0x08 not touched here
    swerve::RayIntersection<CSwerveAssertingErrorHandler> m_rayIntersection;  // 0x14 scratch pick result
    DataOutputStream m_saveStream;                                  // 0x18
    CSwerveGame* m_swerveGame;                                      // 0x28 the running 3D game (NULL: none)
    CGPSMapScreen* m_gpsScreen;                                     // 0x2c
    int m_screenWidth;                                              // 0x30
    int m_screenHeight;                                             // 0x34
    GamePlayManager m_gamePlayManager;                              // 0x38
    com::glu::platform::math::CMatrix2d m_matrix;                   // 0xf8
    int m_field110[2];                                              // 0x110 not touched here
    int m_field118;                                                 // 0x118
    int m_field11c;                                                 // 0x11c
    int m_startTimeMs;                                              // 0x120 creation time in milliseconds
    ConfigReader m_config;                                          // 0x124 dh3d_config.cfg
    bool m_loadingResource;                                         // 0x134 set by CDH_ResourceManager::getResourceData
    swerve::World<CSwerveAssertingErrorHandler> m_auxRender;        // 0x138 BIN_AUX_RENDER
};

#endif
