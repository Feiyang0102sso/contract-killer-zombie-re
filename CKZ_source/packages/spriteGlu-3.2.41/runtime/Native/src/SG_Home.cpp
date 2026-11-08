// SG_Home.cpp - application singleton owning the default SG_Instance (see SG_Home.h). (written by b042)
#include "SG_Home.h"

SG_Home::SG_Home()
    : com::glu::platform::components::CSingleton(SingletonId)
{
}

SG_Home::~SG_Home()
{
    m_instance.Destroy();
}

void SG_Home::Init()
{
    m_instance.Init(NULL);
}

unsigned char SG_Home::LoadQueued(unsigned int timeBudget, unsigned char& finished)
{
    return m_instance.LoadQueued(timeBudget, finished);
}

void SG_Home::QueueArchetypeCharacter(int archetype, int character)
{
    m_instance.QueueArchetypeCharacter(archetype, character, NULL, 0);
}

void SG_Home::DumpArchetypeCharacter(int archetype, int character)
{
    m_instance.DumpArchetypeCharacter(archetype, character);
}

void SG_Home::DumpAllArchetypeCharacters()
{
    m_instance.DumpAllArchetypeCharacters();
}

SG_Instance* SG_Home::GetDefaultSGInstance()
{
    return &m_instance;
}
