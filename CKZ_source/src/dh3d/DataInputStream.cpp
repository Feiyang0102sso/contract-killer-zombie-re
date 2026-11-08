// DataInputStream.cpp - big-endian buffer reader of the game.
#include "DataInputStream.h"
#include "CBinary.h"
#include "Vector_ss.h"

JDataInputStream::JDataInputStream(char* data, int size)
{
    m_data = data;
    m_position = 0;
    m_size = size;
    m_ownsData = false;
}

JDataInputStream::JDataInputStream(com::glu::platform::components::CBinary* binary)
{
    m_data = (char*)binary->m_data;
    m_position = 0;
    m_size = binary->m_size;
}

JDataInputStream::~JDataInputStream()
{
}

char JDataInputStream::readByte()
{
    char value = m_data[m_position];
    m_position++;
    return value;
}

short JDataInputStream::readShort()
{
    int high = readByte();
    int low = readByte() & 0xff;
    return (short)((high << 8) | low);
}

int JDataInputStream::readInt()
{
    int high = readUnsignedShort();
    int low = readUnsignedShort();
    return (high << 16) | low;
}

int JDataInputStream::readUnsignedShort()
{
    return readShort() & 0xffff;
}

char* JDataInputStream::readUTF()
{
    Vector<char> characters;
    char character;
    do {
        character = readByte();
        characters.addElement(character);
    } while (character != 0);

    int length = characters.size();
    char* text = new char[length];
    for (int index = 0; index < length; index++) {
        text[index] = characters[index];
    }
    return text;
}

wchar_t* JDataInputStream::readWChar()
{
    int length = readByte();
    wchar_t* text = new wchar_t[length + 1];
    text[length] = 0;
    for (int index = 0; index < length; index++) {
        text[index] = readShort();
    }
    return text;
}

float JDataInputStream::pow(float value, int exponent)
{
    if (exponent == 0) {
        return 1.0f;
    }
    if (exponent > 0) {
        for (int index = 1; index < exponent; index++) {
            value *= 2.0f;
        }
    } else if (exponent == -127) {
        value = 5.877471754111438e-39f;     // 2^-127 (smallest exponent)
    } else {
        for (int index = exponent; index <= 0; index++) {
            value /= 2.0f;
        }
    }
    return value;
}

float JDataInputStream::readFloat()
{
    int bits = readInt();
    float mantissa = 0.0f;
    unsigned int fraction = (bits & 0x7fffff) | 0x800000;
    unsigned int divisor = 0x800000;
    for (int index = 0; index < 24; index++) {
        mantissa += (float)(fraction & 1) / (float)divisor;
        fraction >>= 1;
        divisor >>= 1;
    }
    float result = mantissa * pow(2.0f, (int)(((unsigned int)bits >> 23) & 0xff) - 127);
    if (bits < 0) {
        result = -result;
    }
    return result;
}
