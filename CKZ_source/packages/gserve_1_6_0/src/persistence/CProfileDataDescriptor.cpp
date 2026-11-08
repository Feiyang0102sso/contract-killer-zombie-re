// CProfileDataDescriptor.cpp - A/B copy selector of a content file collection (b025x).
#include "CProfileDataDescriptor.h"

int CProfileDataDescriptor::getAB()
{
    return m_ab;
}

void CProfileDataDescriptor::switchAB()
{
    // ?: in this order reproduces the original select ('B' preset, 'A' when not 'A')
    m_ab = (m_ab != 'A') ? 'A' : 'B';
}

int CProfileDataDescriptor::targetedCollectionType(int collectionType)
{
    return 1999 - collectionType;
}
