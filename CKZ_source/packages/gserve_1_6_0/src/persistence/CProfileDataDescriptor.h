// CProfileDataDescriptor.h - descriptor collection of a content file collection: remembers which of
// the two server copies ("A" / "B") is current, so a failed upload never destroys the last good one (b025x).
#ifndef CPROFILEDATADESCRIPTOR_H
#define CPROFILEDATADESCRIPTOR_H

#include "CSaveRestoreInterface.h"

class CProfileDataDescriptor : public CSaveRestoreInterface {
public:
    /** 'A' or 'B'. */
    int getAB();
    /** Toggle between 'A' and 'B'. */
    void switchAB();
    /** Collection type of the descriptor that belongs to collection `collectionType` (mirrored below 2000). */
    static int targetedCollectionType(int collectionType);

    int m_ab;       // +0x20 'A' or 'B'
};

#endif
