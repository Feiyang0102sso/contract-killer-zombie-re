// CTip.cpp - localizes the description carried by an XML tip node.
#include "CTip.h"
#include "CXmlHelper.h"
#include "WindowBase.h"

CTip::CTip(TiXmlNode* node)
{
    m_reserved = 0;
    Parse(node);
}

void CTip::Parse(TiXmlNode* node)
{
    if (node != NULL) {
        m_description = Window::ResStringSafe(CXmlHelper::GetAttributeValue(node, "desc"));
    }
}
