// b074: complete GPS map owner, original 115c30..1179f4 (including natural TinyXML weak code).
#include "CBH_GPSMap.h"
#include "CGPSCloud.h"
#include "CEffectsWindow.h"
#include "SGImageWindow.h"
#include "SG_Home.h"
#include "ResMap.h"
#include "tinyxml.h"
#include "CXmlHelper.h"
#include "CSaveManager.h"
#include "CBH_Player.h"
#include "CFiveDaysBonus.h"
#include "CFiveDaysBonusDialog.h"
#include "CDialogQueueWindow.h"
#include "CZombieDialogWindow.h"
#include "CSettingsWindow.h"
#include "CFriendsWindow.h"
#include "CTutorialManager.h"
#include "CTjManager.h"
#include "CAchievementManager.h"
#include "CGCLeaderboardsManager.h"
#include "CIAPCurrencyManager.h"
#include "CDH_WeaponsManager.h"
#include "CPowerUpManager.h"
#include "CGrenadeManager.h"
#include "CShopPackManager.h"
#include "CDrawUtil.h"
#include "DGHelper.h"
#include "SScreenAdaptingConstant.h"
#include "AppSettings.h"
#include "LocalPushManager.h"
#include "CPHInterface.h"
#include "App.h"
#include "CApplet.h"
#include "ICCore.h"
#include "ICRenderSurface.h"
#include "MathLib.h"

using namespace com::glu::platform::graphics;
using namespace com::glu::platform::components;

// b107: owned by this unit per STABS; original value at __TEXT,__const 0x45dce0 (first item of this unit).
extern const int EventId_TargettingMissionFriendSelected;
const int EventId_TargettingMissionFriendSelected = 0x93e847d9;

bool CBH_GPSMap::m_isFirstLoading = true;
// Original merged-data values, at4ff220/230/240; source variable names were lost.
static SScreenAdaptingConstant mapTopLimit(0, 0);
static SScreenAdaptingConstant mapInitialX(-185);
static SScreenAdaptingConstant mapInitialY(-150);

CBH_GPSMap::CBH_GPSMap()
    : Window(false), m_tiles(), m_showFeatured(true), m_minimizes(0),
      m_mapX(mapInitialX()), m_mapY(mapInitialY()), m_dragging(false),
      m_flag100(false), m_bonusElapsed(0), m_fogImage(NULL), m_flag110(false),
      m_scroll(), // Original clears precisely this 44-byte state.
      m_scrollEnabled(true), m_items(), m_selectedItem(NULL)
{
    SetFlags(0x40);
    App::LoadSGArchetype(2, 0);
    App::LoadSGArchetype(1, 0);
    LoadTiles();
    m_scroll.m_state = 0;
    m_scroll.m_x = m_mapX;
    m_scroll.m_y = m_mapY;
}

void CBH_GPSMap::LoadTiles()
{
    m_mapHeight = 0;
    m_mapWidth = 0;
    XString filename = ((App*)WindowApp::m_instance)->m_resMap->getFilename("GPS_MAP_DESCRIPTION");
    char* data = ((App*)WindowApp::m_instance)->m_resMap->loadText(filename);
    TiXmlDocument document;
    document.Parse(data, NULL, TIXML_ENCODING_UNKNOWN);
    if (data != NULL) {
        free(data);
    }
    TiXmlNode* maps = document.FirstChild("maps");
    TiXmlElement* map = (TiXmlElement*)maps->FirstChild("map");
    map->Attribute("width", &m_mapWidth);
    map->Attribute("height", &m_mapHeight);
    map->Attribute("tiles_count_x", &m_tileCountX);
    map->Attribute("tiles_count_y", &m_tileCountY);
}

void CBH_GPSMap::Init()
{
    CreateShop();
    CreateSafehouse();
    CreateEffects();
    Load();
    int targetMedkits = (int)((float)CBH_Player::GetInstance()->GetLevel() * 0.86f);
    CBH_Player* player = CBH_Player::GetInstance();
    int medkits = CBH_Player::GetInstance()->GetStats((PLAYER_STATS_TYPE)5);
    player->AddMedkits(targetMedkits - medkits);
    Save();
    m_barSelected = false;
}

void CBH_GPSMap::CreateShop()
{
    CGPSShop* shop = new CGPSShop();
    shop->SetPos(530.0f, 370.0f);
    shop->SetListener(this);
    if (((App*)WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial() == 1 &&
        ((App*)WindowApp::m_instance)->m_tutorialManager->IsShopAttraction() == 1) {
        shop->SetAttractSpeed(0.4f);
    }
    CGPSItem* item = shop;
    m_items.addElement(item);
}

void CBH_GPSMap::CreateSafehouse()
{
    CGPSSafehouse* safehouse = new CGPSSafehouse();
    safehouse->SetPos(230.0f, 370.0f);
    safehouse->SetListener(this);
    CGPSItem* item = safehouse;
    m_items.addElement(item);
}

void CBH_GPSMap::CreateEffects()
{
    m_effects = new CEffectsWindow(4, 0, WindowApp::m_instance->m_screenWidth, WindowApp::m_instance->m_screenHeight);
    m_effects->SetLayoutType(LAYOUT_NONE);
    AddToFront(m_effects);
    m_fogImage = new SGImageWindow();
    m_fogImage->SetArchetypeCharacter(4, 0);
    m_fogImage->SetAnimation(1, 1);
    m_fogImage->SetLayoutType(LAYOUT_NONE);
    m_fogImage->SetCorner(0, 0);
    m_fogImage->ClearFlags(0x2000);
    m_fogImage->ClearFlags(0x40);
    AddToFront(m_fogImage);
    m_fogY = WindowApp::m_instance->GetRandomValue().Float() * -500.0f;
    int count = WindowApp::m_instance->GetRandomValue().Int(20);
    float baseSpeed = WindowApp::m_instance->GetRandomValue().CFloat() * 0.002f;
    if (count + 10 >= 1) {
        baseSpeed += 0.005f;
        count += 10;
        for (int index = 0; index < count; ++index) {
            float speed = WindowApp::m_instance->GetRandomValue().Float();
            // Original allocates at116086 before the position RNG/Scale/trig calls.
            void* allocation = ::operator new(sizeof(CGPSCloud));
            speed = speed * 0.015f + baseSpeed;
            float randomX = WindowApp::m_instance->GetRandomValue().Float();
            int width = m_mapWidth;
            int scaleX = App::Scale();
            int logicalWidth = width / scaleX;
            float randomY = WindowApp::m_instance->GetRandomValue().Float();
            int height = m_mapHeight;
            int scaleY = App::Scale();
            float cosine = MathLib::Cos(0.0f);
            int logicalHeight = height / scaleY;
            float x = randomX * (float)logicalWidth;
            float velocityX = cosine * speed;
            float y = randomY * (float)logicalHeight;
            float sine = MathLib::Sin(0.0f);
            CGPSCloud* cloud = new (allocation) CGPSCloud(x, y, velocityX, sine * speed);
            cloud->SetMapSize(m_mapWidth, m_mapHeight);
            cloud->m_parallax = WindowApp::m_instance->GetRandomValue().CFloat() * 0.02f + 0.15f;
            cloud->m_offsetX = App::Scale() * -80;
            cloud->m_offsetY = App::Scale() * 50;
            CGPSItem* item = cloud;
            m_items.addElement(item);
        }
    }
}

void CBH_GPSMap::Load()
{
    TiXmlElement* map = ((App*)WindowApp::m_instance)->m_saveManager->GetRootElement()->FirstChildElement("gps_map");
    if (map != NULL) {
        m_showFeatured = CXmlHelper::GetAttributeValueOrUseDefault(map, "show_featured_install", "1").ToBool();
        m_minimizes = CXmlHelper::GetAttributeValueOrUseDefault(map, "FI_minimizes_count", "0").ToInt();
        if (m_isFirstLoading) {
            ++m_minimizes;
        }
        for (int index = 0; index < m_items.m_size; ++index) {
            m_items[index]->Load(map);
        }
    }
    m_isFirstLoading = false;
}

void CBH_GPSMap::Save()
{
    TiXmlElement* root = ((App*)WindowApp::m_instance)->m_saveManager->GetRootElement();
    TiXmlElement* map = CXmlHelper::ObtainElement(root, XString("gps_map"));
    map->Clear();
    map->SetAttribute("show_featured_install", m_showFeatured);
    map->SetAttribute("FI_minimizes_count", m_minimizes);
    for (int index = 0; index < m_items.m_size; ++index) {
        m_items[index]->Save(map);
    }
    ((App*)WindowApp::m_instance)->m_saveManager->SaveOnNextUpdate(0);
}

void CBH_GPSMap::AddGPSItem(CGPSItem* item)
{
    m_items.addElement(item);
}

CBH_GPSMap::~CBH_GPSMap()
{
    Save();
    SAFE_DELETE_VECTOR(m_tiles);
    SAFE_DELETE_VECTOR(m_items);
    SG_Home::GetInstance()->DumpArchetypeCharacter(4, 0);
    SG_Home::GetInstance()->DumpArchetypeCharacter(3, 0);
    SG_Home::GetInstance()->DumpArchetypeCharacter(1, 0);
    SG_Home::GetInstance()->DumpArchetypeCharacter(5, 0);
    SG_Home::GetInstance()->DumpArchetypeCharacter(6, 0);
    SG_Home::GetInstance()->DumpArchetypeCharacter(8, 0);
    ((App*)WindowApp::m_instance)->m_iapCurrencyManager->releaseResources();
    ((App*)WindowApp::m_instance)->m_weaponsManager->releaseResource();
    ((App*)WindowApp::m_instance)->m_powerUpManager->releaseResources();
    ((App*)WindowApp::m_instance)->m_grenadeManager->releaseResources();
    ((App*)WindowApp::m_instance)->m_powerUpPackManager->releaseResources();
    ((App*)WindowApp::m_instance)->m_grenadePackManager->releaseResources();
}

void CBH_GPSMap::Paint(ICGraphics2d& graphics)
{
    if (IsVisible() != 1) {
        return;
    }
    CDrawUtil::Fill(0, 0, WindowApp::m_instance->m_screenWidth, WindowApp::m_instance->m_screenHeight, 0xff000000, (ICGraphics2d::Primitive)3);
    if (m_tiles.m_size >= 1 && m_tileCountY >= 1) {
        int x = m_mapX;
        int y = m_mapY;
        ICRenderSurface* last = NULL;
        for (int row = 0; row < m_tileCountY; ++row) {
            for (int column = 0; column < m_tileCountX; ++column) {
                int index = row * m_tileCountX + column;
                if (index < m_tiles.m_size) {
                    last = m_tiles[index];
                    ((App*)WindowApp::m_instance)->m_helper->DrawImage(last, x, y, 1.0f, 0, 0, 0.0f, 0);
                    x += ImageWidth(last);
                }
            }
            x = m_mapX;
            y += ImageHeight(last);
        }
    }
    for (int index = 0; index < m_items.m_size; ++index) {
        m_items[index]->Paint(graphics, m_mapX, m_mapY);
    }
    Window::Paint(graphics);
}

void CBH_GPSMap::AddMapPatch(ICRenderSurface* surface)
{
    // Original skips NULL surfaces (cbz before addElement).
    if (surface != NULL) {
        m_tiles.addElement(surface);
    }
}

void CBH_GPSMap::OnPointerEvent(PointerEventController::Event event)
{
    int deltaY = event.m_deltaY;
    int absoluteDeltaY = deltaY;
    if (absoluteDeltaY < 0) {
        // The original RSBMI wraps around at 32 bits; INT_MIN stays negative, and signed unary minus must not be used since it introduces UB.
        absoluteDeltaY = (int)(0u - (unsigned int)absoluteDeltaY);
    }
    if (absoluteDeltaY > 4096) {
        deltaY = 0;
    }
    int y = event.m_position.y;
    if (y > m_height || y < 0) {
        y = m_mapY + deltaY;
    }
    int x = event.m_position.x;
    int deltaX = event.m_deltaX;
    if (x > m_width || x < 0) {
        x = m_mapX;
        deltaX = 0;
    }
    if (HasModalChild() || m_dialogQueue->HasModalChild() || ((App*)WindowApp::m_instance)->m_timePauseCount != 0 ||
        ((App*)WindowApp::m_instance)->m_timeScale == 0.0f) {
        return;
    }
    if (event.m_type == PointerEventController::EVENT_MOVE) {
        m_scroll.m_x += (float)deltaX;
        m_scroll.m_y += (float)deltaY;
        if (!m_scrollEnabled) {
            m_scrollEnabled = true;
        }
        float distance = m_mapX - m_pressMapX;
        if (MathLib::Fabs(distance) < (float)(App::Scale() * 15)) {
            distance = m_mapY - m_pressMapY;
            if (MathLib::Fabs(distance) < (float)(App::Scale() * 15)) {
                return;
            }
        }
        if (m_selectedItem != NULL) {
            m_selectedItem->SetPressed(false);
        }
    } else if (event.m_type == PointerEventController::EVENT_UP) {
        m_dragging = false;
        m_scrollEnabled = true;
        if (m_scroll.m_state != 2) {
            m_scroll.m_state = 2;
            m_scroll.m_x = m_mapX;
            m_scroll.m_y = m_mapY;
        }
        float distance = m_mapX - m_pressMapX;
        if (MathLib::Fabs(distance) < (float)(App::Scale() * 15)) {
            distance = m_mapY - m_pressMapY;
            if (MathLib::Fabs(distance) < (float)(App::Scale() * 15)) {
                for (int index = 0; index < m_items.m_size; ++index) {
                    CGPSItem* hit;
                    if (m_items[index]->CheckHit(x - m_mapX, y - m_mapY, &hit) == 1) {
                        hit->OnClick();
                        hit->SetPressed(false);
                        return;
                    }
                }
            }
        }
    } else if (event.m_type == PointerEventController::EVENT_DOWN) {
        m_dragging = true;
        m_pressMapX = m_mapX;
        m_pressMapY = m_mapY;
        m_scrollEnabled = false;
        for (int index = 0; index < m_items.m_size; ++index) {
            CGPSItem* hit;
            if (m_items[index]->CheckHit(x - m_mapX, y - m_mapY, &hit) == 1) {
                if (m_selectedItem != NULL) {
                    m_selectedItem->SetPressed(false);
                }
                m_selectedItem = hit;
                hit->SetPressed(true);
                return;
            }
        }
    }
}

void CBH_GPSMap::Update()
{
    int elapsed = WindowApp::m_instance->m_updateFrameTime;
    CheckMapPosBounds();
    for (int index = 0; index < m_items.m_size; ++index) {
        m_items[index]->Update(elapsed);
    }
    if (m_fogImage != NULL) {
        if (m_fogY < 3000.0f) {
            m_fogY += (float)elapsed * 0.7f;
        }
        m_fogImage->SetCorner(0, (int)m_fogY);
    }
    CTjManager::GetInstance()->CheckFeaturedAppCondition();
    CTjManager::GetInstance()->Update(elapsed);
    if (m_dragging) {
        if (m_scroll.m_state != 0) {
            m_scroll.m_state = 0;
            m_scroll.m_x = m_mapX;
            m_scroll.m_y = m_mapY;
        }
        float inverse = 1.0f / (float)(elapsed + 1);
        m_scroll.m_velocityX = (m_scroll.m_velocityX + (m_scroll.m_x - m_scroll.m_previousX) * inverse) * 0.5f;
        m_scroll.m_velocityY = (m_scroll.m_velocityY + (m_scroll.m_y - m_scroll.m_previousY) * inverse) * 0.5f;
        if (m_scroll.m_velocityX < -1.0f) {
            m_scroll.m_velocityX = -1.0f;
        } else if (m_scroll.m_velocityX > 1.0f) {
            m_scroll.m_velocityX = 1.0f;
        }
        if (m_scroll.m_velocityY < -1.0f) {
            m_scroll.m_velocityY = -1.0f;
        } else if (m_scroll.m_velocityY > 1.0f) {
            m_scroll.m_velocityY = 1.0f;
        }
    } else {
        UpdateScroll();
    }
    m_scroll.m_previousX = m_scroll.m_x;
    m_scroll.m_previousY = m_scroll.m_y;
    m_mapX = (int)m_scroll.m_x;
    m_mapY = (int)m_scroll.m_y;
    if ((m_flags & 0x20000000) != 0) {
        m_bonusElapsed += elapsed;
        ((App*)WindowApp::m_instance)->m_achievementManager->Update(false);
        if (m_bonusElapsed >= 2000) {
            HandleBonuses();
        }
    } else {
        m_bonusElapsed = 0;
    }
    if (m_minimizes >= 1 && m_dialogQueue->m_currentDialog == NULL) {
        if (!CPHInterface::isPublisherContentRequestActive("game_launch")) {
            CPHInterface::startPublisherContentRequest("game_launch", false, false);
        }
        m_minimizes = 0;
    }
}

void CBH_GPSMap::CheckMapPosBounds()
{
    double bound = (double)WindowApp::m_instance->m_screenWidth * 0.6;
    if ((double)m_scroll.m_x > bound) {
        m_scroll.m_x = (float)bound;
    }
    bound = (double)WindowApp::m_instance->m_screenWidth * 0.4;
    if ((double)(m_scroll.m_x + (float)m_mapWidth) < bound) {
        m_scroll.m_x = (float)(bound - (double)m_mapWidth);
    }
    bound = (double)WindowApp::m_instance->m_screenHeight * 0.6;
    if ((double)m_scroll.m_y > bound) {
        m_scroll.m_y = (float)bound;
    }
    bound = (double)WindowApp::m_instance->m_screenHeight * 0.4;
    if ((double)(m_scroll.m_y + (float)m_mapHeight) < bound) {
        m_scroll.m_y = (float)(bound - (double)m_mapHeight);
    }
}

void CBH_GPSMap::UpdateScroll()
{
    int elapsed = WindowApp::m_instance->m_updateFrameTime;
    for (int index = 0; index < m_items.m_size; ++index) {
        float speed = m_items[index]->AttractSpeed();
        if (speed > 0.0f) {
            CRectangle<short> rect = m_items[index]->GetRect();
            short centerX = (unsigned short)rect.m_x + rect.m_width / 2;
            m_scroll.m_targetX = WindowApp::m_instance->m_screenWidth / 2 - centerX;
            rect = m_items[index]->GetRect();
            short centerY = (unsigned short)rect.m_y + rect.m_height / 2;
            m_scroll.m_targetY = WindowApp::m_instance->m_screenHeight / 2 - centerY;
            float lower = WindowApp::m_instance->m_screenWidth - m_mapWidth;
            float target = m_scroll.m_targetX;
            if (target < lower) {
                target = lower;
            } else if (target > 0.0f) {
                target = 0.0f;
            }
            m_scroll.m_targetX = target;
            lower = WindowApp::m_instance->m_screenHeight - m_mapHeight;
            target = m_scroll.m_targetY;
            if (target < lower) {
                target = lower;
            } else if (target > 0.0f) {
                target = 0.0f;
            }
            m_scroll.m_targetY = target;
            m_scroll.m_attractStepX = MathLib::Fabs((m_scroll.m_targetX - (float)m_mapX) * speed);
            m_scroll.m_attractStepY = MathLib::Fabs((m_scroll.m_targetY - (float)m_mapY) * speed);
            m_scroll.m_x = m_mapX;
            m_scroll.m_y = m_mapY;
            m_items[index]->SetAttractSpeed(0.0f);
            if (m_scroll.m_state != 1) {
                m_scroll.m_state = 1;
                m_scroll.m_x = m_mapX;
                m_scroll.m_y = m_mapY;
            }
        }
    }
    float boundsX = 0.0f;
    if (m_mapX >= 1) {
        boundsX = -m_mapX;
    } else {
        int width = WindowApp::m_instance->m_screenWidth;
        int right = m_mapX + m_mapWidth;
        if (right < width && m_mapWidth > width) {
            boundsX = width - right;
        }
    }
    int oldMapY = m_mapY;
    int top = mapTopLimit();
    int currentMapY = m_mapY;
    float boundsY = 0.0f;
    if (oldMapY > top) {
        boundsY = -currentMapY;
    } else {
        int height = WindowApp::m_instance->m_screenHeight;
        int bottom = currentMapY + m_mapHeight;
        if (bottom < height && m_mapHeight > height) {
            boundsY = height - bottom;
        }
    }
    if (m_scroll.m_state == 1) {
        float distanceX = m_scroll.m_targetX - m_scroll.m_x;
        float distanceY = m_scroll.m_targetY - m_scroll.m_y;
        float directionX = 0.0f;
        if (distanceX < 0.0f) {
            directionX = -1.0f;
        } else if (distanceX > 0.0f) {
            directionX = 1.0f;
        }
        float directionY = 0.0f;
        if (distanceY < 0.0f) {
            directionY = -1.0f;
        } else if (distanceY > 0.0f) {
            directionY = 1.0f;
        }
        float stepX = ((directionX * m_scroll.m_attractStepX) * (float)elapsed) / 1000.0f;
        float stepY = ((directionY * m_scroll.m_attractStepY) * (float)elapsed) / 1000.0f;
        if (MathLib::Fabs(stepX) > MathLib::Fabs(distanceX)) {
            stepX = distanceX;
        }
        if (MathLib::Fabs(stepY) > MathLib::Fabs(distanceY)) {
            stepY = distanceY;
        }
        m_scroll.m_x += stepX;
        m_scroll.m_y += stepY;
        if (MathLib::Fabs(m_scroll.m_targetX - (float)m_mapX) < 4.0f) {
            m_scroll.m_attractStepX = 0.0f;
        }
        if (MathLib::Fabs(m_scroll.m_targetY - (float)currentMapY) < 4.0f) {
            m_scroll.m_attractStepY = 0.0f;
        }
        float squared = m_scroll.m_attractStepX * m_scroll.m_attractStepX + m_scroll.m_attractStepY * m_scroll.m_attractStepY;
        if (MathLib::Sqrt(squared) == 0.0f && m_scroll.m_state != 0) {
            m_scroll.m_state = 0;
            m_scroll.m_x = m_mapX;
            m_scroll.m_y = m_mapY;
        }
    } else if (m_scroll.m_state == 2) {
        int firstScale = App::Scale();
        float duration = elapsed + 1;
        int secondScale = App::Scale();
        float velocityX = m_scroll.m_velocityX;
        m_scroll.m_x += velocityX * duration;
        float coefficient = (float)firstScale * 0.0008f;
        float velocityY = m_scroll.m_velocityY;
        m_scroll.m_y += velocityY * duration;
        float denominator = (float)secondScale * 2000.0f;
        float directionX = 0.0f;
        if (velocityX < 0.0f) {
            directionX = -1.0f;
        } else if (velocityX > 0.0f) {
            directionX = 1.0f;
        }
        float decelerationX = ((directionX * coefficient) * duration) * ((denominator + boundsX * boundsX) / denominator);
        if (!(MathLib::Fabs(decelerationX) > MathLib::Fabs(velocityX))) {
            m_scroll.m_velocityX = velocityX - decelerationX;
        } else {
            m_scroll.m_velocityX = 0.0f;
        }
        float directionY = 0.0f;
        if (velocityY < 0.0f) {
            directionY = -1.0f;
        } else if (velocityY > 0.0f) {
            directionY = 1.0f;
        }
        float decelerationY = ((directionY * coefficient) * duration) * ((denominator + boundsY * boundsY) / denominator);
        if (!(MathLib::Fabs(decelerationY) > MathLib::Fabs(velocityY))) {
            m_scroll.m_velocityY = velocityY - decelerationY;
        } else {
            m_scroll.m_velocityY = 0.0f;
        }
        float squared = m_scroll.m_velocityX * m_scroll.m_velocityX + m_scroll.m_velocityY * m_scroll.m_velocityY;
        if ((double)MathLib::Sqrt(squared) < 0.01) {
            m_scroll.m_velocityX = 0.0f;
            m_scroll.m_velocityY = 0.0f;
            if (m_scroll.m_state != 0) {
                m_scroll.m_state = 0;
                m_scroll.m_x = m_mapX;
                m_scroll.m_y = m_mapY;
            }
        }
    }
    float squared = boundsX * boundsX + boundsY * boundsY;
    if (MathLib::Sqrt(squared) > 0.0f) {
        double directionX = 0.0;
        if (boundsX < 0.0f) {
            directionX = -1.0;
        } else if (boundsX > 0.0f) {
            directionX = 1.0;
        }
        double halfTime = (double)elapsed * 0.5;
        float stepX = (float)(halfTime * directionX);
        if (MathLib::Fabs(stepX) > MathLib::Fabs(boundsX)) {
            stepX = boundsX;
        }
        double directionY = 0.0;
        if (boundsY < 0.0f) {
            directionY = -1.0;
        } else if (boundsY > 0.0f) {
            directionY = 1.0;
        }
        float stepY = (float)(halfTime * directionY);
        if (MathLib::Fabs(stepY) > MathLib::Fabs(boundsY)) {
            stepY = boundsY;
        }
        m_scroll.m_x += stepX;
        m_scroll.m_y += stepY;
    }
}

void CBH_GPSMap::HandleBonuses()
{
    bool showTeaser = false;
    if (CBH_Player::GetInstance()->TryMakeFiveDaysBonus(&showTeaser) == 1) {
        ShowFiveDaysBonusMessage();
    } else if (showTeaser) {
        ShowFiveDaysBonusTeaser();
    }
}

void CBH_GPSMap::OnCommand(Event& event)
{
    if (((App*)WindowApp::m_instance)->m_timePauseCount != 0 || ((App*)WindowApp::m_instance)->m_timeScale == 0.0f) {
        return;
    }
    for (int index = 0; index < m_items.m_size; ++index) {
        m_items[index]->OnCommand(event);
        if (event.m_type == Event::NONE) {
            return;
        }
    }
    // Original compare chain tests 0x2ee017b9 first, so the cases are written in reverse.
    if ((unsigned int)event.m_id == 0x97204784) {
        // Original reads the flag as a byte (ldrb; cmp #0).
        if (*(const unsigned char*)&((App*)WindowApp::m_instance)->m_field3c9 == 0) {
            CZombieDialogWindow* dialog = new CZombieDialogWindow(3);
            dialog->SetTitle(Window::ResString("IDS_EXIT_MENU_TITLE"));
            dialog->SetText(Window::ResString("IDS_EXIT_MENU"), true, NULL);
            dialog->AddButton((CZombieDialogWindow::EDialogButton)10, 0x2ee017b9, 0, 0, true, 20);
            dialog->AddButton((CZombieDialogWindow::EDialogButton)4, (unsigned int)-1, 0, 0, true, 20);
            CDialogQueueWindow::AddDialogInQueue(dialog);
            ((App*)WindowApp::m_instance)->m_field3c9 = true;
        }
        event.Clear();
    } else if ((unsigned int)event.m_id == 0x9720b34a) {
        CSettingsWindow* settings = new CSettingsWindow(false, false);
        settings->SetLayoutType(LAYOUT_SIMPLE);
        settings->SetAlign(0x24);
        ((App*)WindowApp::m_instance)->m_mainLayer->AddModal(settings);
        event.Clear();
    } else if ((unsigned int)event.m_id == 0x2ee017b9) {
        CApplet::m_pCore->ExitApp((com::glu::platform::framework::ICCore::eExitStatus)0);
        event.Clear();
    }
}

void CBH_GPSMap::ShowFiveDaysBonusMessage()
{
    int day = ((App*)WindowApp::m_instance)->m_fiveDaysBonus->GetDay();
    CFiveDaysBonusDialog* dialog = new CFiveDaysBonusDialog(day);
    CDialogQueueWindow::AddDialogInQueue(dialog);
}

void CBH_GPSMap::ShowFiveDaysBonusTeaser()
{
    int day = App::GetCurrentDay();
    if (day > CBH_Player::GetInstance()->FiveDaysTeaserDay()) {
        CBH_Player::GetInstance()->FiveDaysTeaserDay() = day;
        CZombieDialogWindow* dialog = new CZombieDialogWindow(3);
        {
            XString title = Window::ResString("IDS_FIVE_DAYS_BONUS_OFFLINE_TITLE");
            XString text = Window::ResString("IDS_FIVE_DAYS_BONUS_OFFLINE_TEXT");
            dialog->InitOkMessage(title, text, false, NULL);
        }
        m_parent->AddModal(dialog);
    }
}

void CBH_GPSMap::Shutdown()
{
    SetupLocalPushNotifications();
}

void CBH_GPSMap::SetupLocalPushNotifications()
{
    static const char* pushes[] = {
        "IDS_LOCAL_PUSH_1", "IDS_LOCAL_PUSH_2", "IDS_LOCAL_PUSH_3", "IDS_LOCAL_PUSH_4", "IDS_LOCAL_PUSH_5"
    };
    LocalPushManager::CancelLocalPushNotifications(false);
    if (((App*)WindowApp::m_instance)->m_settings->IsLocalPushEnabled() != 1) {
        return;
    }
    int index = WindowApp::m_instance->GetRandomValue().Int(5);
    XString text = Window::ResString(pushes[index]);
    {
        XString::AnsiString message(text.m_data);
        // The button text is a temporary of the call (original stack slots / destruction order).
        LocalPushManager::SetLocalPushNotification(message.m_text,
            XString::AnsiString(Window::ResString("IDS_LOCAL_PUSH_BUTTON_TEXT").m_data).m_text, 86400, 1);
    }
    {
        XString::AnsiString message(text.m_data);
        // The button text is a temporary of the call (original stack slots / destruction order).
        LocalPushManager::SetLocalPushNotification(message.m_text,
            XString::AnsiString(Window::ResString("IDS_LOCAL_PUSH_BUTTON_TEXT").m_data).m_text, 259200, 2);
    }
}

void CBH_GPSMap::OnBarItemSelected(CGPSBar*)
{
    m_barSelected = true;
    CFriendsWindow* friends = new CFriendsWindow();
    ((App*)WindowApp::m_instance)->m_mainLayer->AddModal(friends);
}

void CBH_GPSMap::ShowSelectFriendDialog()
{
    // Original 1177a8 is return-only.
}

void CBH_GPSMap::OnPause()
{
    SetupLocalPushNotifications();
}

void CBH_GPSMap::OnEnterBackground()
{
    ++m_minimizes;
}

void CBH_GPSMap::OnEnterForeground()
{
    ((App*)WindowApp::m_instance)->m_leaderboardsManager->ReloadLeaderboards();
}
