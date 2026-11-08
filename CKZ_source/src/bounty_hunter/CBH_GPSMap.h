// PROVISIONAL (created by b048): owner src/bounty_hunter/CBH_GPSMap.cpp not written yet
// CBH_GPSMap.h - the map screen window; only what src/dh3d uses is declared.
// b074: historical provisional note retained; full owner ABI and true listener bases restored.
#ifndef CBH_GPSMAP_H
#define CBH_GPSMAP_H

#include "WindowBase.h"
#include "PointerEventController.h"
#include "CGPSItem.h"
#include "CGPSShop.h"
#include "CGPSSafehouse.h"
#include "CGPSBar.h"

class CEffectsWindow;
class SGImageWindow;
class CDialogQueueWindow;

class CBH_GPSMap : public Window, public CGPSShopListener,
    public CGPSBarListener, public CGPSSafehouseListener {
public:
    CBH_GPSMap();
    virtual ~CBH_GPSMap();
    virtual void Init();
    virtual void Shutdown();
    virtual void Paint(com::glu::platform::graphics::ICGraphics2d& graphics);
    virtual void Update();
    virtual void OnCommand(Event& event);
    virtual void OnPause();
    virtual void OnEnterBackground();
    virtual void OnEnterForeground();
    virtual void OnBarItemSelected(CGPSBar* bar);
    /** Gesture from the PointerEventController of the map screen. */
    void OnPointerEvent(PointerEventController::Event event);
    void LoadTiles();
    void CreateShop();
    void CreateSafehouse();
    void CreateEffects();
    void Load();
    void Save();
    void AddGPSItem(CGPSItem* item);
    void AddMapPatch(com::glu::platform::graphics::ICRenderSurface* surface);
    void CheckMapPosBounds();
    void UpdateScroll();
    void HandleBonuses();
    void ShowFiveDaysBonusMessage();
    void ShowFiveDaysBonusTeaser();
    void SetupLocalPushNotifications();
    void ShowSelectFriendDialog();
    static bool m_isFirstLoading;

    struct ScrollState {
        int m_state;          // relative 00, overall114
        float m_velocityX;    // 04
        float m_velocityY;    // 08
        float m_attractStepX; // 0c
        float m_attractStepY; // 10
        float m_targetX;      // 14
        float m_targetY;      // 18
        float m_x;            // 1c
        float m_y;            // 20
        float m_previousX;    // 24
        float m_previousY;    // 28
    };
    Vector<com::glu::platform::graphics::ICRenderSurface*> m_tiles; // b8
    bool m_showFeatured; // c8
    int m_minimizes;     // cc
    int m_mapWidth;      // d0
    int m_mapHeight;     // d4
    int m_tileCountX;    // d8
    int m_tileCountY;    // dc
    int m_mapX;          // e0
    int m_mapY;          // e4
    int m_pressMapX;     // e8
    int m_pressMapY;     // ec
    int m_unresolvedF0;  // f0, not initialized by original ctor
    bool m_dragging;     // f4
    int m_unresolvedF8[2]; // f8/fc, not initialized by original ctor
    bool m_flag100;      // 100
    unsigned int m_bonusElapsed; // 104
    CEffectsWindow* m_effects;    // 108
    SGImageWindow* m_fogImage;    // 10c
    bool m_flag110;              // 110
    ScrollState m_scroll;        // 114, original memset clears exactly 44B
    bool m_scrollEnabled;        // 140
    Vector<CGPSItem*> m_items;    // 144
    CGPSItem* m_selectedItem;     // 154
    int m_unresolved158;         // 158
    // The original GPSGame ctor 11a99e writes DialogQueue; 116bdc reads its current dialog +bc.
    CDialogQueueWindow* m_dialogQueue; // 15c, the former thin view misidentified it as GPSHud.
    float m_fogY;                // 160
    bool m_barSelected;          // 164, total168
};

#endif
