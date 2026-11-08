// CSingleton.h - registers an object in the application singleton table for its lifetime
// (created by b005, header-only).
#ifndef CSINGLETON_H
#define CSINGLETON_H

#include "CClass.h"
#include "CApplet.h"
#include "CApp.h"
#include "CHash.h"

namespace com { namespace glu { namespace platform { namespace components {

/** Singletons derive from CClass: the class id (+0x04) doubles as the key in CApp::m_singletons.
    Constructor and destructor are explicitly inline (b003: needed for the original EH cleanup shape). */
class CSingleton : public core::CClass {
public:
    inline explicit CSingleton(unsigned int singletonId)
    {
        m_classId = singletonId;
        CApplet::m_pApp->m_singletons->Insert(singletonId, this);
    }

    inline virtual ~CSingleton()
    {
        CApplet::m_pApp->m_singletons->Remove(m_classId);
    }
};

} } } }

#endif
