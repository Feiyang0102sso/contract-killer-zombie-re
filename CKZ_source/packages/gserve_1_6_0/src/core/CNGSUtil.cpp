// CNGSUtil.cpp - helpers of the gServe code.
// The Base64 part is the classic public-domain implementation; it uses the C library heap
// (calloc / free), not the np_* wrappers of the prefix header.
#undef calloc
#undef free
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <libkern/OSByteOrder.h>
#include "CNGSUtil.h"
#include "CObjectMap.h"
#include "CObjectMapValue.h"
#include "ICStdUtil.h"


using com::glu::platform::components::CStrChar;
using com::glu::platform::components::CStrWChar;
using com::glu::platform::network::sDataBlock;

/** Characters needed for the decimal text of an int (with sign and terminator). */
static const int INTEGER_TEXT_SIZE = 12;

/** Result codes of the obfuscation helpers. */
static const unsigned char OBFUSCATE_OK = 0;
static const unsigned char OBFUSCATE_BAD_INPUT = 1;
static const unsigned char OBFUSCATE_NO_MEMORY = 2;

static char encode(unsigned char value);
static unsigned char decode(char character);

/** True for the characters of the Base64 alphabet and the padding '='. */
static inline int is_base64(char character)
{
    if (character >= 'A' && character <= 'Z') {
        return 1;
    }
    if (character >= 'a' && character <= 'z') {
        return 1;
    }
    if (character >= '0' && character <= '9') {
        return 1;
    }
    if (character == '+' || character == '/' || character == '=') {
        return 1;
    }
    return 0;
}

char* CNGSUtil::Base64Encode(const unsigned char* data, unsigned int length)
{
    char* text = NULL;
    if (data == NULL) {
        return text;
    }
    if (length == 0) {
        length = strlen((const char*)data);
    }
    text = (char*)calloc(sizeof(char), length * 4 / 3 + 4);
    char* output = text;
    for (unsigned int index = 0; index < length; index += 3) {
        unsigned char byte1 = 0;
        unsigned char byte2 = 0;
        unsigned char byte3 = 0;
        byte1 = data[index];
        if (index + 1 < length) {
            byte2 = data[index + 1];
        }
        if (index + 2 < length) {
            byte3 = data[index + 2];
        }
        unsigned char sextet1 = byte1 >> 2;
        unsigned char sextet2 = ((byte1 & 0x3) << 4) | (byte2 >> 4);
        unsigned char sextet3 = ((byte2 & 0xf) << 2) | (byte3 >> 6);
        unsigned char sextet4 = byte3 & 0x3f;
        *output++ = encode(sextet1);
        *output++ = encode(sextet2);
        if (index + 1 < length) {
            *output++ = encode(sextet3);
        } else {
            *output++ = '=';
        }
        if (index + 2 < length) {
            *output++ = encode(sextet4);
        } else {
            *output++ = '=';
        }
    }
    *output = '\0';
    return text;
}

/** Base64 digit of a 6-bit value. */
static char encode(unsigned char value)
{
    if (value < 26) {
        return 'A' + value;
    }
    if (value < 52) {
        return 'a' + (value - 26);
    }
    if (value < 62) {
        return '0' + (value - 52);
    }
    if (value == 62) {
        return '+';
    }
    return '/';
}

int CNGSUtil::Base64Decode(unsigned char* output, const unsigned char* text, int length)
{
    if (text == NULL || *text == '\0') {
        return 0;
    }
    unsigned char* destination = output;
    int bufferSize = strlen((const char*)text) + 1;
    char* digits = (char*)calloc(sizeof(char), bufferSize);

    // keep only the Base64 characters
    int digitCount = 0;
    for (int index = 0; text[index] != '\0'; index++) {
        if (is_base64(text[index])) {
            digits[digitCount++] = text[index];
        }
    }

    for (int index = 0; index < digitCount; index += 4) {
        char digit1 = 'A';
        char digit2 = 'A';
        char digit3 = 'A';
        char digit4 = 'A';
        digit1 = digits[index];
        if (index + 1 < digitCount) {
            digit2 = digits[index + 1];
        }
        if (index + 2 < digitCount) {
            digit3 = digits[index + 2];
        }
        if (index + 3 < digitCount) {
            digit4 = digits[index + 3];
        }
        unsigned char value1 = decode(digit1);
        unsigned char value2 = decode(digit2);
        unsigned char value3 = decode(digit3);
        unsigned char value4 = decode(digit4);
        *destination++ = (value1 << 2) | (value2 >> 4);
        if (digit3 != '=') {
            *destination++ = ((value2 & 0xf) << 4) | (value3 >> 2);
        }
        if (digit4 != '=') {
            *destination++ = ((value3 & 0x3) << 6) | value4;
        }
    }
    free(digits);
    *destination = '\0';
    return destination - output;
}

/** 6-bit value of a Base64 digit. */
static unsigned char decode(char character)
{
    if (character >= 'A' && character <= 'Z') {
        return character - 'A';
    }
    if (character >= 'a' && character <= 'z') {
        return character - 'a' + 26;
    }
    if (character >= '0' && character <= '9') {
        return character - '0' + 52;
    }
    if (character == '+') {
        return 62;
    }
    return 63;
}

CStrChar CNGSUtil::WStrToCStr(const CStrWChar& text)
{
    char* narrow = new char[text.GetLength() + 1];
    int length = text.GetLength();
    if (length > 0) {
        size_t converted = wcstombs(narrow, text.GetWCharPtr(), length + 1);
        if (converted == (size_t)-1) {
            narrow[0] = '\0';
        } else {
            narrow[length] = '\0';
        }
    }
    narrow[text.GetLength()] = '\0';
    CStrChar result;
    result += narrow;
    if (narrow != NULL) {
        delete[] narrow;
    }
    return result;
}

void CNGSUtil::IntegerToString(int value, CStrWChar& text)
{
    wchar_t digits[INTEGER_TEXT_SIZE] = {0};
    com::glu::platform::core::ICStdUtil::SWPrintF(digits, L"%i", value);
    text = digits;
}

CStrWChar CNGSUtil::GetMessageDataString(CObjectMap* message, const CStrWChar& path, const CStrWChar& separator)
{
    CStrWChar result;
    CObjectMapValue* value = message->getDataAt(path, separator);
    if (value != NULL && value->getType() == CObjectMapValue::TYPE_STRING) {
        CStrWChar text = ((CObjectMapString*)value)->m_value;
        result = text.GetWCharPtr();
    }
    return result;
}

unsigned char CNGSUtil::ObfuscateString(sDataBlock& block, const wchar_t* text, const wchar_t* key)
{
    unsigned int* data = NULL;
    unsigned int size = 0;
    unsigned char result = OBFUSCATE_OK;
    if (text != NULL) {
        if (key != NULL) {
            data = NULL;
            unsigned int textLength = wcslen(text);
            unsigned int keyLength = wcslen(key);
            result = OBFUSCATE_BAD_INPUT;
            if (textLength != 0 && keyLength != 0) {
                // [textLength, keyLength, key..., text...]
                size = (keyLength + textLength) * sizeof(unsigned int) + 2 * sizeof(unsigned int);
                data = (unsigned int*)malloc(size);
                if (data == NULL) {
                    result = OBFUSCATE_NO_MEMORY;
                    size = 0;
                } else {
                    memset(data, 0, size);
                    data[0] = textLength;
                    data[1] = keyLength;
                    unsigned int* textPart = data + 2 + keyLength;
                    for (unsigned int index = 0; index < textLength; index++) {
                        textPart[index] = key[(index + 1) % keyLength] ^ text[index];
                    }
                    for (unsigned int index = 0; index < keyLength; index++) {
                        data[index + 2] = textPart[index % textLength] ^ key[index];
                    }
                    unsigned int* character = textPart;
                    for (unsigned int index = 0; index < textLength; index++) {
                        *character = OSSwapInt16(*character);
                        character++;
                    }
                    for (unsigned int index = 0; index < keyLength; index++) {
                        data[index + 2] = OSSwapInt16(data[index + 2]);
                    }
                    result = OBFUSCATE_OK;
                }
            }
        }
    }
    block.m_size = size;
    block.m_data = data;
    return result;
}

unsigned char CNGSUtil::DeObfuscateString(sDataBlock& block, const wchar_t* text, const wchar_t* key,
                                          unsigned int textLength, unsigned int keyLength)
{
    unsigned char result = OBFUSCATE_OK;
    unsigned int* data = NULL;
    unsigned int size = 0;
    if (text != NULL && key != NULL) {
        data = NULL;
        result = OBFUSCATE_BAD_INPUT;
        if (textLength != 0 && keyLength != 0) {
            // `text` is [key..., text...] as stored by ObfuscateString (after its two length words)
            size = (keyLength + textLength) * sizeof(unsigned int);
            data = (unsigned int*)malloc(size);
            if (data == NULL) {
                result = OBFUSCATE_NO_MEMORY;
                size = 0;
            } else {
                memcpy(data, text, size);
                unsigned int* keyPart = data;
                unsigned int* textPart = data + keyLength;
                for (unsigned int index = 0; index < keyLength; index++) {
                    keyPart[index] = OSSwapInt16(keyPart[index]);
                }
                for (unsigned int index = 0; index < textLength; index++) {
                    textPart[index] = OSSwapInt16(textPart[index]);
                }
                for (unsigned int index = 0; index < keyLength; index++) {
                    keyPart[index] = textPart[index % textLength] ^ keyPart[index];
                }
                for (unsigned int index = 0; index < textLength; index++) {
                    textPart[index] = keyPart[(index + 1) % keyLength] ^ textPart[index];
                }
                result = OBFUSCATE_OK;
            }
        }
    }
    block.m_size = size;
    block.m_data = data;
    return result;
}
