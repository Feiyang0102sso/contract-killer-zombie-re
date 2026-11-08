// PROVISIONAL (created by b057x): owner src/bounty_hunter/Manager/CVisualCharacterTypeManager.cpp not written yet
// CVisualCharacterTypeManager.h - visual character types; only what src/general/App.cpp uses is declared.
#ifndef CVISUALCHARACTERTYPEMANAGER_H
#define CVISUALCHARACTERTYPEMANAGER_H

#include <stddef.h>
#include "xString.h"  // added by b077
#include "HashTable.h"
#include "tws.h"
#include "CSwerve.h"

class CVisualCharacterType;  // added by b077

class CVisualCharacterTypeManager {
public:
    CVisualCharacterTypeManager();
    virtual ~CVisualCharacterTypeManager();
    /** The type named `name`. */  // added by b077
    CVisualCharacterType* GetType(const XString& name) const;  // added by b077

    /** Drop the model resources and pooled copies of all types. */
    void CleanupResources() const;

    // b079e: owner restored; the older PROVISIONAL note above describes its first declaration.
    HashTable<XString, CVisualCharacterType*> m_types;             // 0x04
    swerve::Group<CSwerveAssertingErrorHandler> m_commonGroup;     // 0x10 shadow RootNode
};

#endif
