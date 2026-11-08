// ConfigReader.h - "token" "value" pairs read from a text resource.
#ifndef CONFIGREADER_H
#define CONFIGREADER_H

#include <stddef.h>
#include <wchar.h>
#include "Vector_ss.h"

class ConfigReader {
public:
    /** One token / value pair; both strings are owned. */
    struct DataUnit {
        char* m_token;      // 0x00
        char* m_value;      // 0x04

        ~DataUnit()
        {
            if (m_token != NULL) {
                delete[] m_token;
                m_token = NULL;
            }
            if (m_value != NULL) {
                delete[] m_value;
                m_value = NULL;
            }
        }
    };

    /** Load and parse the text resource `fileName`. */
    ConfigReader(const wchar_t* fileName);
    ~ConfigReader();

    /** Parse `length` characters of `text` into token / value pairs (the separator between them is skipped). */
    void init(const char* text, int length);
    /**
     * Next whitespace separated or "quoted" word at *position; its length goes to *tokenLength and
     * *position moves past it (and past the closing quote).
     */
    const char* getNextQuotedString(const char* text, int length, int* tokenLength, int* position);

    /** Value of `token`, NULL when absent. */
    const char* GetStringValueByToken(const char* token, bool required);
    /** Value of `token` as an integer, 0 when absent. */
    int GetIntegerValueByToken(const char* token, bool required);
    /** Value of `token` as a float, 0 when absent. */
    float GetFloatValueByToken(const char* token, bool required);

    Vector<DataUnit*> m_units;      // 0x00
};

#endif
