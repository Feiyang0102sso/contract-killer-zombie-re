// CWaveManager.cpp - configured wave libraries, filtering and shop recommendations.
#include "CWaveManager.h"
#include "App.h"
#include "ResMap.h"
#include "CXmlHelper.h"
#include "tinyxml.h"
#include "CBH_PersonTypeManager.h"
#include "CPersonsGroupManager.h"
#include "CShopItem.h"
#include "CRandGen.h"

using com::glu::platform::core::CRandGen;

CWaveManager::CWaveManager()
{
    const wchar_t* cbFiles[3];
    cbFiles[0] = L"res/common/configs/waves_cb_e.xml";
    cbFiles[1] = L"res/common/configs/waves_cb_m.xml";
    cbFiles[2] = L"res/common/configs/waves_cb_h.xml";
    for (int index = 0; index < 3; index++) {
        Load(cbFiles[index], &m_cb[index]);
    }
    const wchar_t* icFiles[3];
    icFiles[0] = L"res/common/configs/waves_ic_e.xml";
    icFiles[1] = L"res/common/configs/waves_ic_m.xml";
    icFiles[2] = L"res/common/configs/waves_ic_h.xml";
    for (int index = 0; index < 3; index++) {
        Load(icFiles[index], &m_ic[index]);
    }
    const wchar_t* kaFiles[3];
    kaFiles[0] = L"res/common/configs/waves_ka_e.xml";
    kaFiles[1] = L"res/common/configs/waves_ka_m.xml";
    kaFiles[2] = L"res/common/configs/waves_ka_h.xml";
    for (int index = 0; index < 3; index++) {
        Load(kaFiles[index], &m_ka[index]);
    }
    const wchar_t* stFiles[3];
    stFiles[0] = L"res/common/configs/waves_st_e.xml";
    stFiles[1] = L"res/common/configs/waves_st_m.xml";
    stFiles[2] = L"res/common/configs/waves_st_h.xml";
    for (int index = 0; index < 3; index++) {
        Load(stFiles[index], &m_st[index]);
    }
    Load(L"res/common/configs/waves_quest.xml", &m_quest);
    Load(L"res/common/configs/waves_em.xml", &m_em);
    ReloadRecommendations();
}

void CWaveManager::Load(const wchar_t* path, Vector<Vector<SWave> >* waves)
{
    char* text = ((App*)WindowApp::m_instance)->m_resMap->loadText(XString(path));
    TiXmlDocument document;
    document.Parse(text, NULL, TIXML_ENCODING_UNKNOWN);
    if (text != NULL) {
        free(text);
    }
    // l: full TinyXML interface; original 14032c/140344 are still direct inline reads of firstChild/next.
    // The original keeps a guard plus a rotated loop here.
    TiXmlNode* root = document.FirstChild();
    if (root != NULL) {
        do {
            if (root->FirstChild("wave") != NULL) {
                Parse(root, waves);
                break;
            }
            root = root->NextSibling();
        } while (root != NULL);
    }
}

void CWaveManager::ReloadRecommendations()
{
    SAFE_DELETE_VECTOR(m_recommendations);
    char* text = ((App*)WindowApp::m_instance)->m_resMap->loadText(XString(L"res/common/configs/waves_recommendations.xml"));
    TiXmlDocument document;
    document.Parse(text, NULL, TIXML_ENCODING_UNKNOWN);
    if (text != NULL) {
        free(text);
    }
    TiXmlNode* root = document.FirstChild("recommendations");
    for (TiXmlNode* node = root->FirstChild("rec"); node != NULL; node = node->NextSibling("rec")) {
        SWaveRecommendation* recommendation = new SWaveRecommendation();
        recommendation->m_id = CXmlHelper::GetAttributeValue(node, "id").ToInt();
        for (TiXmlNode* item = node->FirstChild("item"); item != NULL; item = item->NextSibling("item")) {
            recommendation->m_items.addElement(CXmlHelper::GetAttributeValue(item, "name"));
        }
        m_recommendations.addElement(recommendation);
    }
}

void CWaveManager::Parse(TiXmlNode* root, Vector<Vector<SWave> >* waves)
{
    for (int level = 0; level < 101; level++) {
        waves->addElement(Vector<SWave>());
    }
    // The original fetches the next sibling before the per-wave locals are destroyed.
    TiXmlNode* node = root->FirstChild("wave");
    while (node != NULL) {
        SWave wave;
        wave.m_id = CXmlHelper::GetAttributeValue(node, "id").ToInt();
        wave.m_level = CXmlHelper::GetAttributeValue(node, "level").ToInt();
        wave.m_filter = CXmlHelper::GetAttributeValue(node, "filter");
        wave.m_cashXpMultiplier = CXmlHelper::GetAttributeValueOrUseDefault(node, "mod_cash_xp", XString(0)).ToFloat();
        wave.m_personMultiplier = CXmlHelper::GetAttributeValueOrUseDefault(node, "mod_zombie_health", XString(0)).ToFloat();
        wave.m_threshold = CXmlHelper::GetAttributeValueOrUseDefault(node, "treshold", XString(100)).ToFloat();
        wave.m_pause = CXmlHelper::GetAttributeValueOrUseDefault(node, "pause", XString(0)).ToFloat();
        wave.m_recommendationId = CXmlHelper::GetAttributeValueOrUseDefault(node, "recommendation_id", XString(0)).ToInt();
        wave.m_mustBeClear = CXmlHelper::GetAttributeValueOrUseDefault(node, "must_be_clear", "0").ToBool();
        for (int index = 1; index < 5; index++) {
            XString sectorName = L"sector" + XString(index);
            XString typeName = L"type" + XString(index);
            XString sizeName = L"size" + XString(index);
            SSpawnEvent event;
            event.m_sector = CXmlHelper::GetAttributeValue(node, XString::AnsiString(sectorName.c_str()));
            event.m_type = CXmlHelper::GetAttributeValue(node, XString::AnsiString(typeName.c_str()));
            event.m_count = CXmlHelper::GetAttributeValue(node, XString::AnsiString(sizeName.c_str())).ToInt();
            if (event.m_count != 0) {
                ((App*)WindowApp::m_instance)->m_personTypeManager->TypeIsExists(event.m_type);
                ((App*)WindowApp::m_instance)->m_personsGroupManager->GroupIsExists(event.m_type);
                wave.m_events.addElement(event);
            }
        }
        if (wave.m_events.size() != 0) {
            (*waves)[wave.m_level].addElement(wave);
        }
        node = node->NextSibling("wave");
    }
}

CWaveManager::SWave* CWaveManager::GetWave(CMission::EMissionType type, CMission::EMissionDifficulty difficulty, int id)
{
    Vector<Vector<SWave> >* waves;
    switch (type) {
    case CMission::MISSION_KA:
        switch (difficulty) {
        case CMission::DIFFICULTY_ANY: waves = &m_ka[CRandGen::GetInstance()->GetRand(3)]; break;
        case CMission::DIFFICULTY_EASY: waves = &m_ka[0]; break;
        case CMission::DIFFICULTY_MEDIUM: waves = &m_ka[1]; break;
        case CMission::DIFFICULTY_HARD: waves = &m_ka[2]; break;
        default: return NULL;
        }
        break;
    case CMission::MISSION_IC:
        switch (difficulty) {
        case CMission::DIFFICULTY_ANY: waves = &m_ic[CRandGen::GetInstance()->GetRand(3)]; break;
        case CMission::DIFFICULTY_EASY: waves = &m_ic[0]; break;
        case CMission::DIFFICULTY_MEDIUM: waves = &m_ic[1]; break;
        case CMission::DIFFICULTY_HARD: waves = &m_ic[2]; break;
        default: return NULL;
        }
        break;
    case CMission::MISSION_CB:
        switch (difficulty) {
        case CMission::DIFFICULTY_ANY: waves = &m_cb[CRandGen::GetInstance()->GetRand(3)]; break;
        case CMission::DIFFICULTY_EASY: waves = &m_cb[0]; break;
        case CMission::DIFFICULTY_MEDIUM: waves = &m_cb[1]; break;
        case CMission::DIFFICULTY_HARD: waves = &m_cb[2]; break;
        default: return NULL;
        }
        break;
    case CMission::MISSION_ST:
        switch (difficulty) {
        case CMission::DIFFICULTY_ANY: waves = &m_st[CRandGen::GetInstance()->GetRand(3)]; break;
        case CMission::DIFFICULTY_EASY: waves = &m_st[0]; break;
        case CMission::DIFFICULTY_MEDIUM: waves = &m_st[1]; break;
        case CMission::DIFFICULTY_HARD: waves = &m_st[2]; break;
        default: return NULL;
        }
        break;
    case CMission::MISSION_QU: waves = &m_quest; break;
    case CMission::MISSION_EM: waves = &m_em; break;
    default: return NULL;
    }
    return GetWave(waves, id);
}

CWaveManager::SWave* CWaveManager::GetWave(Vector<Vector<SWave> >* waves, int id)
{
    for (int level = 0; level < waves->size(); level++) {
        Vector<SWave>& choices = (*waves)[level];
        for (int index = 0; index < choices.size(); index++) {
            if (choices[index].m_id == id) {
                return &choices[index];
            }
        }
    }
    return NULL;
}

CWaveManager::SWave* CWaveManager::GetRandomWave(CMission::EMissionType type, CMission::EMissionDifficulty difficulty,
                                               int level, const XString& filter)
{
    Vector<Vector<SWave> >* waves;
    switch (type) {
    case CMission::MISSION_KA:
        switch (difficulty) {
        case CMission::DIFFICULTY_ANY: waves = &m_ka[CRandGen::GetInstance()->GetRand(3)]; break;
        case CMission::DIFFICULTY_EASY: waves = &m_ka[0]; break;
        case CMission::DIFFICULTY_MEDIUM: waves = &m_ka[1]; break;
        case CMission::DIFFICULTY_HARD: waves = &m_ka[2]; break;
        default: return NULL;
        }
        break;
    case CMission::MISSION_IC:
        switch (difficulty) {
        case CMission::DIFFICULTY_ANY: waves = &m_ic[CRandGen::GetInstance()->GetRand(3)]; break;
        case CMission::DIFFICULTY_EASY: waves = &m_ic[0]; break;
        case CMission::DIFFICULTY_MEDIUM: waves = &m_ic[1]; break;
        case CMission::DIFFICULTY_HARD: waves = &m_ic[2]; break;
        default: return NULL;
        }
        break;
    case CMission::MISSION_CB:
        switch (difficulty) {
        case CMission::DIFFICULTY_ANY: waves = &m_cb[CRandGen::GetInstance()->GetRand(3)]; break;
        case CMission::DIFFICULTY_EASY: waves = &m_cb[0]; break;
        case CMission::DIFFICULTY_MEDIUM: waves = &m_cb[1]; break;
        case CMission::DIFFICULTY_HARD: waves = &m_cb[2]; break;
        default: return NULL;
        }
        break;
    case CMission::MISSION_ST:
        switch (difficulty) {
        case CMission::DIFFICULTY_ANY: waves = &m_st[CRandGen::GetInstance()->GetRand(3)]; break;
        case CMission::DIFFICULTY_EASY: waves = &m_st[0]; break;
        case CMission::DIFFICULTY_MEDIUM: waves = &m_st[1]; break;
        case CMission::DIFFICULTY_HARD: waves = &m_st[2]; break;
        default: return NULL;
        }
        break;
    case CMission::MISSION_QU: return GetRandomWave(&m_quest, 0, 9999, filter);
    case CMission::MISSION_EM: waves = &m_em; break;
    default: return NULL;
    }
    return GetRandomWave(waves, level, filter);
}

CWaveManager::SWave* CWaveManager::GetRandomWave(Vector<Vector<SWave> >* waves, int level, const XString& filter)
{
    Vector<SWave>& choices = (*waves)[level];
    if (filter.ByteSize() == 0) {
        return &choices[CRandGen::GetInstance()->GetRand(choices.size())];
    }
    Vector<int> matching;
    for (int index = 0; index < choices.size(); index++) {
        if (ApplyFilter(choices[index].m_filter, filter) == true) {
            matching.addElement(index);
        }
    }
    if (matching.size() != 0) {
        return &choices[matching[CRandGen::GetInstance()->GetRand(matching.size())]];
    }
    return NULL;
}

CWaveManager::SWave* CWaveManager::GetRandomWave(Vector<Vector<SWave> >* waves, int firstLevel, int lastLevel, const XString& filter)
{
    Vector<SWave*> matching;
    int end = lastLevel;
    if (waves->size() - 1 < end) {
        end = waves->size() - 1;
    }
    int begin = firstLevel;
    if (begin < 0) {
        begin = 0;
    }
    // Original @0x140ea6 excludes this clamped upper bound.
    for (int level = begin; level < end; level++) {
        Vector<SWave>& choices = (*waves)[level];
        if (filter.ByteSize() == 0) {
            for (int index = 0; index < choices.size(); index++) {
                SWave* wave = &choices[index];
                matching.addElement(wave);
            }
        } else {
            for (int index = 0; index < choices.size(); index++) {
                if (ApplyFilter(choices[index].m_filter, filter) == true) {
                    SWave* wave = &choices[index];
                    matching.addElement(wave);
                }
            }
        }
    }
    if (matching.size() != 0) {
        return matching[CRandGen::GetInstance()->GetRand(matching.size())];
    }
    return NULL;
}

CItem* CWaveManager::GetRecommendedItem(int id)
{
    for (int index = 0; index < m_recommendations.size(); index++) {
        if (m_recommendations[index]->m_id == id) {
            for (int itemIndex = 0; itemIndex < m_recommendations[index]->m_items.size(); itemIndex++) {
                CItem* item = (CItem*)App::GetItemByStringUId(m_recommendations[index]->m_items[itemIndex]);
                if (item != NULL && item->isAvailable() == false && item->IsLocked() != true) {
                    return item;
                }
            }
        }
    }
    return NULL;
}

CWaveManager::~CWaveManager()
{
    SAFE_DELETE_VECTOR(m_recommendations);
    // Original explicitly empties these arrays before their automatic destructors run.
    for (int index = 0; index < 3; index++) {
        m_ic[index].removeAllElements();
        m_ka[index].removeAllElements();
        m_cb[index].removeAllElements();
        m_st[index].removeAllElements();
    }
}
