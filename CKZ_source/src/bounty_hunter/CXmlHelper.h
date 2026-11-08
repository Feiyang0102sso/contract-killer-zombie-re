// PROVISIONAL (created by b077): owner src/bounty_hunter/CXmlHelper.cpp not written yet
// CXmlHelper.h - attribute / element helpers over TinyXML used by the game configs.
// Signatures from the symbol names; return types from the callers (sret XString, element pointers).
// b074i: full recovery of the 9 ranges; the PROVISIONAL above is kept as history.
#ifndef CXMLHELPER_H
#define CXMLHELPER_H

#include "xString.h"
#include "tinyxml.h"

class CXmlHelper {
public:
    /** Attribute `name` of `node` (empty when missing). */
    static XString GetAttributeValue(const TiXmlNode* node, const char* name);
    /** Attribute `name` of `node`, empty without complaining when missing. */
    static XString GetAttributeValueOptional(const TiXmlNode* node, const char* name);
    static XString GetAttributeValueOrUseDefault(const TiXmlNode* node, const char* name, const char* defaultValue);
    static XString GetAttributeValueOrUseDefault(const TiXmlNode* node, const char* name, XString defaultValue);
    static int GetIntAttributeValueOrUseDefault(const TiXmlNode* node, const char* name, int defaultValue);
    static bool GetBoolAttributeValueOrUseDefault(const TiXmlNode* node, const char* name, bool defaultValue);
    /** Child element `name` of `parent`, created when missing. */
    static TiXmlElement* ObtainElement(TiXmlElement* parent, const XString& name);
    /** Child element `name` of `parent` (NULL when missing). */
    static TiXmlElement* GetElement(TiXmlElement* parent, const XString& name);
    /** Original 122930 returns the full document in place via the hidden r0; the resource buffer is released after parsing. */
    static TiXmlDocument Parse(const wchar_t* resourceName);
};

#endif
