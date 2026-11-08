// CNGSJSONParser.cpp - JSON tokenizer/decoder into CObjectMapValue trees and the matching encoder.
#include "CNGSJSONParser.h"

#include <wchar.h>
#include "CWStringBuffer.h"
#include "CObjectMap.h"
#include "CObjectMapValue.h"
#include "CObjectMapObject.h"
#include "CObjectMapArray.h"

using namespace com::glu::platform;
using namespace com::glu::platform::components;

/** Single-character tokens. */
static const wchar_t tokens[] = { L'{', L'}', L'[', L']', L',', L':' };
/** Characters that start / continue a number. */
static const wchar_t numbers[] = { L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7', L'8', L'9' };
/** Characters skipped between tokens. */
static const wchar_t whitespaces[] = { L' ', L'\t', L'\r', L'\n' };
/** Escape letters, printed[i] stands for specials[i]. */
static const wchar_t printed[] = { L'r', L'n', L't', L'\\', L'"', L'f', L'b' };
/** Characters written as a backslash escape. */
static const wchar_t specials[] = { L'\r', L'\n', L'\t', L'\\', L'"', L'\f', L'\b' };

/** Tokenizer states. */
enum {
    TOKEN_NONE = 0,     // between tokens
    TOKEN_STRING = 1,   // inside a quoted string
    TOKEN_NUMBER = 2,
    TOKEN_LITERAL = 3   // true / false / null (or anything else)
};

/** `character` is one of the first `count` entries of `set`. */
static inline bool isOneOf(wchar_t character, const wchar_t* set, unsigned int count)
{
    unsigned int index = 0;
    do {
        if (set[index] == character) {
            return true;
        }
        index++;
    } while (index < count);
    return false;
}

/** `text` without its first and last character (the quotes); empty when nothing is left. */
static inline CStrWChar stripQuotes(const CStrWChar& text)
{
    if (text.m_length < 3) {
        return CStrWChar();
    }
    return text.GetSubString(1, text.m_length - 2);
}

CStrWChar CNGSJSONParser::decodeString(const CStrWChar& text)
{
    CStrWChar result;
    int length = text.m_length;
    if (length > 0) {
        wchar_t* buffer = new wchar_t[length + 1];
        unsigned char escaped = 0;
        int outLength = 0;
        for (int index = 0; index < length; index++) {
            if (escaped) {
                for (unsigned int special = 0; special < 7; special++) {
                    if (text.m_string[index] == printed[special]) {
                        buffer[outLength] = specials[special];
                        outLength++;
                        break;
                    }
                }
                escaped = 0;
            } else if (text.m_string[index] == L'\\') {
                escaped = 1;
            } else {
                buffer[outLength] = text.m_string[index];
                outLength++;
            }
        }
        buffer[outLength] = 0;
        result = CStrWChar(buffer);
        if (buffer != NULL) {
            delete[] buffer;
        }
    }
    return result;
}

CStrWChar CNGSJSONParser::encodeString(const CStrWChar& text)
{
    CStrWChar result;
    int length = text.m_length;
    if (length > 0) {
        // worst case: every character escaped
        wchar_t* buffer = new wchar_t[length * 2 + 1];
        int outLength = 0;
        for (int index = 0; index < length; index++) {
            wchar_t character = text.m_string[index];
            bool isSpecial = false;
            for (unsigned int special = 0; special < 7; special++) {
                if (character == specials[special]) {
                    buffer[outLength] = L'\\';
                    buffer[outLength + 1] = printed[special];
                    outLength += 2;
                    isSpecial = true;
                    break;
                }
            }
            if (!isSpecial) {
                buffer[outLength] = character;
                outLength++;
            }
        }
        buffer[outLength] = 0;
        result = CStrWChar(buffer);
        if (buffer != NULL) {
            delete[] buffer;
        }
    }
    return result;
}

CStrWChar CNGSJSONParser::encodeMessages(const core::CVector<CObjectMap>& messages)
{
    CWStringBuffer buffer(L"{\"payload\":[");
    for (int index = 0; index < messages.Size(); index++) {
        if (index > 0) {
            buffer.append(L", ");
        }
        buffer.append(L"{\"version\":");
        // the version field (+0x0c) is read as an unsigned value
        encodeInteger(buffer, (unsigned int)messages[index].m_state);
        buffer.append(L", \"sequenceId\":");
        encodeInteger(buffer, messages[index].m_messageId);
        buffer.append(L", \"message\":");
        encodeValue(buffer, messages[index].getRoot());
        buffer.append(L"}");
    }
    buffer.append(L"]}");
    return CStrWChar(buffer.getBuffer());
}

void CNGSJSONParser::encodeInteger(CWStringBuffer& buffer, long long value)
{
    if (value != 0) {
        // digits are produced least significant first
        wchar_t digits[24];
        int digitCount = 0;
        long long magnitude = value;
        if (value < 0) {
            magnitude = -value;
        }
        while (magnitude > 0) {
            digits[digitCount] = L'0' + magnitude % 10;
            digitCount++;
            magnitude = magnitude / 10;
        }
        if (value < 0) {
            digits[digitCount] = L'-';
            digitCount++;
        }
        while (digitCount > 0) {
            digitCount--;
            buffer.append(digits[digitCount]);
        }
    } else {
        buffer.append(L'0');
    }
}

void CNGSJSONParser::encodeValue(CWStringBuffer& buffer, const CObjectMapValue* value)
{
    if (value == NULL) {
        return;
    }

    switch (value->getType()) {
    case CObjectMapValue::TYPE_OBJECT: {
        const CObjectMapObject* object = (const CObjectMapObject*)value;
        int count = object->m_values.Size();
        buffer.append(L'{');
        for (int index = 0; index < count; index++) {
            if (index > 0) {
                buffer.append(L", ");
            }
            buffer.append(L'"');
            // the name goes through a temporary copy
            buffer.append(CStrWChar(object->m_keys[index]).m_string);
            buffer.append(L"\": ");
            encodeValue(buffer, object->m_values[index]);
        }
        buffer.append(L'}');
        break;
    }
    case CObjectMapValue::TYPE_ARRAY: {
        const CObjectMapArray* array = (const CObjectMapArray*)value;
        int count = array->m_values.Size();
        buffer.append(L'[');
        for (int index = 0; index < count; index++) {
            if (index > 0) {
                buffer.append(L", ");
            }
            encodeValue(buffer, array->m_values[index]);
        }
        buffer.append(L']');
        break;
    }
    case CObjectMapValue::TYPE_BOOL:
        if (((const CObjectMapBool*)value)->m_value) {
            buffer.append(L"true");
        } else {
            buffer.append(L"false");
        }
        break;
    case CObjectMapValue::TYPE_INT:
        encodeInteger(buffer, ((const CObjectMapInt*)value)->m_value);
        break;
    case CObjectMapValue::TYPE_STRING:
        buffer.append(L'"');
        // the value is escaped from a temporary copy
        buffer.append(encodeString(CStrWChar(((const CObjectMapString*)value)->m_value)).m_string);
        buffer.append(L'"');
        break;
    case CObjectMapValue::TYPE_NULL:
        buffer.append(L"null");
        break;
    }
}

CStrWChar CNGSJSONParser::encodeValue(const CObjectMapValue* value)
{
    CWStringBuffer buffer(128);
    encodeValue(buffer, value);
    return CStrWChar(buffer.getBuffer());
}

int CNGSJSONParser::decodeMessages(const CStrWChar& json, core::CVector<CObjectMap>& messages)
{
    int decodedCount = 0;
    CObjectMapValue* root = decodeValue(json);
    if (root != NULL) {
        if (root->getType() == CObjectMapValue::TYPE_OBJECT) {
            CObjectMapValue* payload = ((CObjectMapObject*)root)->getEntry(CStrWChar(L"payload"));
            if (payload != NULL && payload->getType() == CObjectMapValue::TYPE_ARRAY) {
                CObjectMapArray* entries = (CObjectMapArray*)payload;
                for (int index = 0; index < entries->m_values.Size(); index++) {
                    CObjectMapValue* entry = entries->m_values[index];
                    if (entry == NULL || entry->getType() != CObjectMapValue::TYPE_OBJECT) {
                        continue;
                    }
                    CObjectMapObject* envelope = (CObjectMapObject*)entry;
                    CObjectMapValue* version = envelope->getEntry(CStrWChar(L"version"));
                    CObjectMapValue* sequenceId = envelope->getEntry(CStrWChar(L"sequenceId"));
                    if (version == NULL || sequenceId == NULL) {
                        continue;
                    }
                    CObjectMapValue* message = envelope->getEntry(CStrWChar(L"message"));
                    if (message == NULL || message->getType() != CObjectMapValue::TYPE_OBJECT) {
                        continue;
                    }
                    CObjectMap* decoded = new CObjectMap(message);
                    decoded->m_messageId = (unsigned short)((CObjectMapInt*)sequenceId)->m_value;
                    messages.AddAll(messages.Size(), decoded, 1);
                    delete decoded;
                    decodedCount++;
                }
            }
        }
        if (root != NULL) {
            delete root;
        }
    }
    return decodedCount;
}

CObjectMapValue* CNGSJSONParser::decodeValue(const CStrWChar& json)
{
    CObjectMapValue* root = NULL;
    core::CVector<CStrWChar> tokenList;
    if (!tokenize(tokenList, json)) {
        return root;
    }

    // name of the next object member, set by a string token in an object
    CStrWChar key;
    if (tokenList[0].m_string[0] == L'[') {
        root = new CObjectMapArray();
    } else if (tokenList[0].m_string[0] == L'{') {
        root = new CObjectMapObject();
    } else {
        return root;
    }

    CObjectMapValue* current = root;
    int index = 1;
    do {
        const CStrWChar& token = tokenList[index];
        wchar_t first = token.m_string[0];
        CObjectMapValue* next = current;

        if (first == L'{' || first == L'[') {
            // nested container
            int keyLength = key.m_length;
            int type = current->getType();
            CObjectMapValue* child;
            if (keyLength == 0) {
                if (type != CObjectMapValue::TYPE_ARRAY) {
                    if (root != NULL) {
                        delete root;
                        root = NULL;
                    }
                    break;
                }
                if (token.m_string[0] == L'{') {
                    CObjectMapObject* object = new CObjectMapObject();
                    object->m_parent = current;
                    child = object;
                } else {
                    CObjectMapArray* array = new CObjectMapArray();
                    array->m_parent = current;
                    child = array;
                }
                ((CObjectMapArray*)current)->m_values.Add(child);
            } else {
                if (type != CObjectMapValue::TYPE_OBJECT) {
                    if (root != NULL) {
                        delete root;
                        root = NULL;
                    }
                    break;
                }
                if (token.m_string[0] == L'{') {
                    CObjectMapObject* object = new CObjectMapObject();
                    object->m_parent = current;
                    child = object;
                } else {
                    CObjectMapArray* array = new CObjectMapArray();
                    array->m_parent = current;
                    child = array;
                }
                ((CObjectMapObject*)current)->addEntry(stripQuotes(key), child);
                key = "";
            }
            next = child;
        } else if (first == L'}') {
            if (current->getType() != CObjectMapValue::TYPE_OBJECT) {
                if (root != NULL) {
                    delete root;
                    root = NULL;
                }
                break;
            }
            next = ((CObjectMapObject*)current)->m_parent;
        } else if (first == L']') {
            if (current->getType() != CObjectMapValue::TYPE_ARRAY) {
                if (root != NULL) {
                    delete root;
                    root = NULL;
                }
                break;
            }
            next = ((CObjectMapArray*)current)->m_parent;
        } else if (first == L',' || first == L':') {
            // separators carry no information
        } else if (first == L'"') {
            if (current->getType() == CObjectMapValue::TYPE_ARRAY) {
                CStrWChar text = stripQuotes(token);
                ((CObjectMapArray*)current)->m_values.Add(new CObjectMapString(decodeString(text)));
            } else if (key.m_length != 0) {
                CStrWChar text = stripQuotes(token);
                ((CObjectMapObject*)current)->addEntry(stripQuotes(key), new CObjectMapString(decodeString(text)));
                key = "";
            } else {
                // first string in an object member: the name
                key = token;
                continue;
            }
        } else if (first == L'-' || isOneOf(first, numbers, 10)) {
            if (current->getType() == CObjectMapValue::TYPE_ARRAY) {
                ((CObjectMapArray*)current)->m_values.Add(new CObjectMapInt(decodeInteger(token)));
            } else if (key.m_length != 0) {
                ((CObjectMapObject*)current)->addEntry(stripQuotes(key), new CObjectMapInt(decodeInteger(token)));
                key = "";
            } else {
                if (root != NULL) {
                    delete root;
                    root = NULL;
                }
                break;
            }
        } else if (token == L"true" || token == L"false") {
            unsigned char boolValue = (token == L"true");
            if (current->getType() == CObjectMapValue::TYPE_ARRAY) {
                ((CObjectMapArray*)current)->m_values.Add(new CObjectMapBool(boolValue));
            } else if (key.m_length != 0) {
                ((CObjectMapObject*)current)->addEntry(stripQuotes(key), new CObjectMapBool(boolValue));
                key = "";
            } else {
                if (root != NULL) {
                    delete root;
                    root = NULL;
                }
                break;
            }
        } else if (token == L"null") {
            if (current->getType() == CObjectMapValue::TYPE_ARRAY) {
                ((CObjectMapArray*)current)->m_values.Add(new CObjectMapNull());
            } else if (key.m_length != 0) {
                ((CObjectMapObject*)current)->addEntry(stripQuotes(key), new CObjectMapNull());
                key = "";
            } else {
                if (root != NULL) {
                    delete root;
                    root = NULL;
                }
                break;
            }
        } else {
            if (root != NULL) {
                delete root;
                root = NULL;
            }
            break;
        }

        // closing the outermost container ends the document
        current = next;
        if (current == NULL) {
            break;
        }
    } while (++index < tokenList.Size());
    return root;
}

int CNGSJSONParser::tokenize(core::CVector<CStrWChar>& tokenList, CStrWChar json)
{
    // open brackets, innermost last
    core::CVector<wchar_t> brackets;
    int position = 0;
    unsigned char escaped = 0;
    unsigned char valid = 1;
    int state = TOKEN_NONE;
    int tokenStart = 0;
    wchar_t character[2] = { 0, 0 };

    do {
        character[0] = json.m_string[position];
        switch (state) {
        case TOKEN_NONE:
            if (isOneOf(character[0], tokens, 6)) {
                tokenList.Add(CStrWChar(character));
                if (character[0] == L'{' || character[0] == L'[') {
                    brackets.Add(character[0]);
                } else if (character[0] == L'}' || character[0] == L']') {
                    // '{' / '[' are two code points below their closing bracket
                    if (brackets.Size() > 0 && character[0] - 2 == brackets[brackets.Size() - 1]) {
                        brackets.RemoveRange(brackets.Size() - 1, brackets.Size());
                    } else {
                        valid = 0;
                    }
                }
            } else if (character[0] == L'"') {
                state = TOKEN_STRING;
                tokenStart = position;
            } else if (character[0] == L'-' || isOneOf(character[0], numbers, 10)) {
                state = TOKEN_NUMBER;
                tokenStart = position;
            } else if (!isOneOf(character[0], whitespaces, 4)) {
                state = TOKEN_LITERAL;
                tokenStart = position;
            }
            break;
        case TOKEN_STRING:
            if (escaped) {
                escaped = 0;
            } else if (character[0] == L'\\') {
                escaped = 1;
            } else if (character[0] == L'"') {
                // the token keeps its quotes
                tokenList.Add(json.GetSubString(tokenStart, position));
                state = TOKEN_NONE;
            }
            break;
        case TOKEN_NUMBER:
            if (!isOneOf(character[0], numbers, 10)) {
                tokenList.Add(json.GetSubString(tokenStart, position - 1));
                state = TOKEN_NONE;
                // the terminating character is looked at again
                continue;
            }
            break;
        case TOKEN_LITERAL:
            if (isOneOf(character[0], whitespaces, 4) || isOneOf(character[0], tokens, 6)) {
                tokenList.Add(json.GetSubString(tokenStart, position - 1));
                state = TOKEN_NONE;
                continue;
            }
            break;
        }
        position++;
    } while (position < json.m_length);

    // every bracket must be closed
    if (valid && brackets.Size() > 0) {
        valid = 0;
    }
    return valid;
}

long long CNGSJSONParser::decodeInteger(const CStrWChar& text)
{
    bool negative = (text.m_string[0] == L'-');
    int start = 0;
    if (negative) {
        start = 1;
    }
    long long value = 0;
    for (int index = start; index < text.m_length; index++) {
        value = value * 10 + text.m_string[index] - L'0';
    }
    if (negative) {
        value = -value;
    }
    return value;
}
