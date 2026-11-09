// CPersonsGroupManager.cpp - person groups and randomized spawn selections.
#include "CPersonsGroupManager.h"
#include "CXmlHelper.h"
#include "App.h"
#include "ResMap.h"
#include "DGHelper.h"
#include "CBH_PersonTypeManager.h"
#include "CRandGen.h"

using com::glu::platform::core::CRandGen;

CPersonsGroupManager::CPersonsGroupManager()
{
    ResMap* resources = ((App*)WindowApp::m_instance)->m_resMap;
    char* text = resources->loadText(XString(L"res/common/configs/persons_groups.xml"));
    TiXmlDocument document;
    document.Parse(text);
    delete[] text;
    Parse(document.FirstChild("persons_groups"));
}

void CPersonsGroupManager::Parse(TiXmlNode* node)
{
    for (TiXmlNode* child = node->FirstChild("persons_group"); child != NULL; child = child->NextSibling("persons_group")) {
        SPersonsGroup group;
        group.m_name = CXmlHelper::GetAttributeValue(child, "name");
        XString delay = CXmlHelper::GetAttributeValueOptional(child, "delayMS");
        if (delay.ByteSize() != 0) {
            Vector<XString> limits;
            DGHelper::SplitStringAndTrim(limits, delay, XString("-"));
            int delayMs = limits[0].ToInt();
            group.m_minDelayMs = delayMs;
            if (limits.size() > 1) {
                delayMs = limits[1].ToInt();
            }
            group.m_maxDelayMs = delayMs;
        }
        for (TiXmlNode* setNode = child->FirstChild("persons_set"); setNode != NULL; setNode = setNode->NextSibling("persons_set")) {
            SPersonsSet set;
            {
                XString persons = CXmlHelper::GetAttributeValue(setNode, "persons");
                Vector<XString> names;
                DGHelper::SplitStringAndTrim(names, persons, XString(","));
                set.m_persons = names;
                for (int index = 0; index < set.m_persons.size(); index++) {
                    ((App*)WindowApp::m_instance)->m_personTypeManager->GetType(set.m_persons[index]);
                }
            }
            {
                XString count = CXmlHelper::GetAttributeValue(setNode, "count");
                Vector<XString> limits;
                DGHelper::SplitStringAndTrim(limits, count, XString("-"));
                int countValue = limits[0].ToInt();
                set.m_minCount = countValue;
                if (limits.size() > 1) {
                    countValue = limits[1].ToInt();
                }
                set.m_maxCount = countValue;
            }
            group.m_sets.addElement(set);
        }
        m_groups.addElement(group);
    }
}

bool CPersonsGroupManager::GroupIsExists(const XString& name)
{
    for (int index = 0; index < m_groups.size(); index++) {
        if (m_groups[index].m_name == name) {
            return true;
        }
    }
    return false;
}

CPersonsGroupManager::SPersonsGroup* CPersonsGroupManager::GetGroup(const XString& name)
{
    for (int index = 0; index < m_groups.size(); index++) {
        if (m_groups[index].m_name == name) {
            return &m_groups[index];
        }
    }
    XString diagnostic = XString::Format(L"Can't find person group with name \"%s\"", name.c_str());
    // Preserve the first-element fallback, including NULL when no storage exists.
    return m_groups.m_items;
}

void CPersonsGroupManager::GeneratePersonsSelection(Vector<SSpawnablePerson>& persons, const SPersonsGroup& group,
                                                   const CWaveManager::SWave* wave)
{
    for (int index = 0; index < group.m_sets.size(); index++) {
        const SPersonsSet& set = group.m_sets[index];
        int count = CRandGen::GetInstance()->GetRandRange(set.m_minCount, set.m_maxCount);
        for (int remaining = count; remaining > 0; remaining--) {
            int selected = CRandGen::GetInstance()->GetRandRange(0, set.m_persons.size() - 1);
            SSpawnablePerson person;
            person.m_typeName = set.m_persons[selected];
            person.m_seed = CRandGen::GetInstance()->Generate();
            if (wave != NULL) {
                person.m_waveMultiplier1 = wave->m_personMultiplier;
                person.m_waveMultiplier2 = person.m_waveMultiplier1;
            }
            persons.addElement(person);
        }
    }
}

CPersonsGroupManager::~CPersonsGroupManager()
{
    // The vector member owns and destroys all nested records.
}
