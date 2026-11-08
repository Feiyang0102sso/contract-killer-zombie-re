// ConfigReader.cpp - token / value configuration files of the game.
#include "ConfigReader.h"
#include <string.h>
#include <stdlib.h>
#include "App.h"
#include "ResMap.h"
#include "xString.h"

void ConfigReader::init(const char* text, int length)
{
    int tokenLength = 0;
    int position = 0;
    do {
        const char* tokenText = getNextQuotedString(text, length, &tokenLength, &position);
        if (position < length) {
            char* token = new char[tokenLength + 1];
            memcpy(token, tokenText, tokenLength);
            token[tokenLength] = 0;

            // the separator between the token and its value
            getNextQuotedString(text, length, &tokenLength, &position);

            const char* valueText = getNextQuotedString(text, length, &tokenLength, &position);
            char* value = new char[tokenLength + 1];
            memcpy(value, valueText, tokenLength);
            value[tokenLength] = 0;

            DataUnit* unit = new DataUnit;
            unit->m_token = token;
            unit->m_value = value;
            m_units.addElement(unit);
        }
    } while (position < length);
}

ConfigReader::ConfigReader(const wchar_t* fileName)
{
    ResMap* resMap = ((App*)WindowApp::m_instance)->m_resMap;
    char* text = resMap->loadText(XString(fileName));
    init(text, strlen(text));
    if (text != NULL) {
        delete[] text;
    }
}

ConfigReader::~ConfigReader()
{
    for (int index = 0; index < m_units.size(); index++) {
        if (m_units[index] != NULL) {
            delete m_units[index];
            m_units[index] = NULL;
        }
    }
}

const char* ConfigReader::getNextQuotedString(const char* text, int length, int* tokenLength, int* position)
{
    while (*position < length && text[*position] <= ' ') {
        (*position)++;
    }
    int end;
    if (text[*position] == '"') {
        (*position)++;
        end = *position;
        while (end < length && text[end] != '"') {
            end++;
        }
    } else {
        end = *position;
        while (end < length && text[end] > ' ') {
            end++;
        }
    }
    *tokenLength = end - *position;
    const char* start = text + *position;
    if (end < length && text[end] == '"') {
        end++;
    }
    *position = end;
    return start;
}

const char* ConfigReader::GetStringValueByToken(const char* token, bool required)
{
    for (int index = 0; index < m_units.size(); index++) {
        DataUnit* unit = m_units[index];
        if (strcmp(token, unit->m_token) == 0) {
            return unit->m_value;
        }
    }
    return NULL;
}

int ConfigReader::GetIntegerValueByToken(const char* token, bool required)
{
    int result = 0;
    const char* value = GetStringValueByToken(token, false);
    if (value != NULL) {
        result = atoi(value);
    }
    return result;
}

float ConfigReader::GetFloatValueByToken(const char* token, bool required)
{
    const char* value = GetStringValueByToken(token, false);
    if (value == NULL) {
        return 0.0f;
    }
    return XString(value).ToFloat();
}
