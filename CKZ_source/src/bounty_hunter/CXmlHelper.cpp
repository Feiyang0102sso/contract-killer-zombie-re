// CXmlHelper.cpp: original 122650..12299a, keeps the TinyXML virtual calls and the temporary string lifetimes.
#include "CXmlHelper.h"
#include "App.h"
#include "ResMap.h"
#include <stdlib.h>

XString CXmlHelper::GetAttributeValue(const TiXmlNode* node, const char* name)
{
    return XString(node->ToElement()->Attribute(name)).Replace(L"\\n", L"\n", true);
}

XString CXmlHelper::GetAttributeValueOptional(const TiXmlNode* node, const char* name)
{
    const char* value = node->ToElement()->Attribute(name);
    if (value == NULL) {
        return XString();
    }
    return XString(value).Replace(L"\\n", L"\n", true);
}

XString CXmlHelper::GetAttributeValueOrUseDefault(const TiXmlNode* node, const char* name, const char* defaultValue)
{
    return GetAttributeValueOrUseDefault(node, name, XString(defaultValue));
}

XString CXmlHelper::GetAttributeValueOrUseDefault(const TiXmlNode* node, const char* name, XString defaultValue)
{
    const char* value = node->ToElement()->Attribute(name);
    if (value == NULL) {
        return defaultValue;
    }
    return XString(value).Replace(L"\\n", L"\n", true);
}

int CXmlHelper::GetIntAttributeValueOrUseDefault(const TiXmlNode* node, const char* name, int defaultValue)
{
    if (node == NULL) {
        return defaultValue;
    }
    const char* value = node->ToElement()->Attribute(name);
    if (value == NULL) {
        return defaultValue;
    }
    return XString(value).Replace(L"\\n", L"\n", true).ToInt();
}

bool CXmlHelper::GetBoolAttributeValueOrUseDefault(const TiXmlNode* node, const char* name, bool defaultValue)
{
    if (node == NULL) {
        return defaultValue;
    }
    const char* value = node->ToElement()->Attribute(name);
    if (value == NULL) {
        return defaultValue;
    }
    return XString(value).Replace(L"\\n", L"\n", true).ToBool();
}

TiXmlElement* CXmlHelper::ObtainElement(TiXmlElement* parent, const XString& name)
{
    TiXmlNode* found;
    {
        XString::AnsiString ansi(name.c_str());
        found = parent->FirstChild(ansi);
    }
    if (found == NULL) {
        // The temporary Ansi is destroyed first at the end of the construction statement; element is destroyed only after it is cloned.
        TiXmlElement element((const char*)XString::AnsiString(name.c_str()));
        return parent->InsertEndChild(element)->ToElement();
    }
    // The original still repeats the lookup on a hit; do not cache found in place of the second call.
    XString::AnsiString ansi(name.c_str());
    return parent->FirstChild(ansi)->ToElement();
}

TiXmlElement* CXmlHelper::GetElement(TiXmlElement* parent, const XString& name)
{
    XString::AnsiString ansi(name.c_str());
    return parent->FirstChildElement(ansi);
}

TiXmlDocument CXmlHelper::Parse(const wchar_t* resourceName)
{
    char* text = ((App*)WindowApp::m_instance)->m_resMap->loadText(XString(resourceName));
    TiXmlDocument document;
    document.Parse(text, NULL, TIXML_ENCODING_UNKNOWN);
    if (text != NULL) {
        free(text);
    }
    return document;
}
