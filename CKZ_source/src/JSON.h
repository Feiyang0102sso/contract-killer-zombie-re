// JSON.h - small owning JSON tree used by CGameApp's push notification parser.
// The original header path is unknown; types and methods follow the original CGameApp ranges.
#ifndef GAME_JSON_H
#define GAME_JSON_H

#include <wchar.h>
#include "CString.h"

class JSONObject;
class JSONArray;
class JSONString;

class JSONValue {
public:
    virtual ~JSONValue() {}
    virtual JSONObject* IsObject() { return NULL; }
    virtual JSONArray* IsArray() { return NULL; }
    virtual JSONString* IsString() { return NULL; }

    static void Cleanup(CString<wchar_t>& text);
    static CString<wchar_t> ExtractFirstItem(CString<wchar_t>& text);
    static CString<wchar_t> ExtractKey(CString<wchar_t>& text);
    static JSONValue* Parse(CString<wchar_t> text);
};

class JSONString : public JSONValue {
public:
    JSONString(CString<wchar_t> text)
    {
        text.Trim(L' ');
        text.Trim(L'"');
        m_text = text;
    }
    virtual ~JSONString() {}
    virtual JSONString* IsString() { return this; }

    CString<wchar_t> m_text;   // 0x04
};

class JSONArray : public JSONValue {
public:
    JSONArray(CString<wchar_t> text)
    {
        text.Trim(L'[');
        text.Trim(L']');
        while (text.Length() != 0) {
            CString<wchar_t> item = ExtractFirstItem(text);
            JSONValue* value = Parse(item);
            m_values.Add(value);
        }
    }
    virtual ~JSONArray()
    {
        for (int index = 0; index < m_values.Size(); index++) {
            delete m_values[index];
        }
    }
    virtual JSONArray* IsArray() { return this; }

    com::glu::platform::core::CVector<JSONValue*> m_values;  // 0x04
};

class JSONObject : public JSONValue {
public:
    JSONObject(CString<wchar_t> text)
    {
        text.Trim(L'{');
        text.Trim(L'}');
        while (text.Length() != 0) {
            CString<wchar_t> item = ExtractFirstItem(text);
            CString<wchar_t> key = ExtractKey(item);
            JSONString* name = new JSONString(key);
            JSONValue* value = Parse(item);
            m_keys.Add(name);
            m_values.Add(value);
        }
    }
    virtual ~JSONObject()
    {
        for (int index = 0; index < m_keys.Size(); index++) {
            delete m_keys[index];
        }
        for (int index = 0; index < m_values.Size(); index++) {
            delete m_values[index];
        }
    }
    virtual JSONObject* IsObject() { return this; }

    JSONValue* GetValue(const wchar_t* key)
    {
        for (int index = 0; index < m_keys.Size(); index++) {
            if (wcscmp(key, m_keys[index]->m_text.m_characters.m_data) == 0) {
                return m_values[index];
            }
        }
        return NULL;
    }

    com::glu::platform::core::CVector<JSONString*> m_keys;    // 0x04
    com::glu::platform::core::CVector<JSONValue*> m_values;  // 0x1c
};

#endif
