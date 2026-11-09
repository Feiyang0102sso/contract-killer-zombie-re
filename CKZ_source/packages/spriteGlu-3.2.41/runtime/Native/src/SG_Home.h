// SG_Home.h - application singleton owning the default SG_Instance. (created by b042)
#ifndef SG_HOME_H
#define SG_HOME_H

#include "CSingleton.h"
#include "SG_Instance.h"

class SG_Home : public com::glu::platform::components::CSingleton {
public:
    /** Key in the application singleton table. */
    enum { SingletonId = 0x272be9b5 };

    SG_Home();
    virtual ~SG_Home();

    /** The application's instance, created on first use. */
    static inline SG_Home* GetInstance()
    {
        void* found = NULL;
        CApplet::m_pApp->m_singletons->Find(SingletonId, &found);
        SG_Home* home = static_cast<SG_Home*>(found);
        if (home == NULL) {
            home = new SG_Home();
        }
        return home;
    }

    void Init();
    unsigned char LoadQueued(unsigned int timeBudget, unsigned char& finished);
    void QueueArchetypeCharacter(int archetype, int character);
    void DumpArchetypeCharacter(int archetype, int character);
    void DumpAllArchetypeCharacters();
    SG_Instance* GetDefaultSGInstance();

    SG_Instance m_instance;     // 0x08
};

#endif
