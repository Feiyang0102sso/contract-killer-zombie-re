// CBH_SaveManager.cpp - only the shared save document survives in CKZ 3.1.1.
// The original unit's own methods were dead-stripped by the linker (nothing calls
// them); what remains is the static member CBH_SaveManager::m_xml and its static
// initializer (TiXmlDocument constructor + atexit destructor). The TinyXML inline
// functions this unit also emitted are matched from CSaveManager.o.
#include "tinyxml.h"

/** Save manager of the original; only its static XML document is kept. */
class CBH_SaveManager {
public:
    static TiXmlDocument m_xml;
};

TiXmlDocument CBH_SaveManager::m_xml;
