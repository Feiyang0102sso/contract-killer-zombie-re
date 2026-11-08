// DataOutputStream.cpp - big-endian buffer writer of the game.
#include "DataOutputStream.h"

DataOutputStream::DataOutputStream()
{
}

void DataOutputStream::writeByte(unsigned char value)
{
    m_buffer.addElement(value);
}

void DataOutputStream::writeInt(int value)
{
    writeByte(value >> 24);
    writeByte(value >> 16);
    writeByte(value >> 8);
    writeByte(value);
}

void DataOutputStream::writeWStr(const wchar_t* text, wchar_format_t format, wchar_t terminator)
{
    int length = wcslen(text);
    switch (format) {
    case WCHAR_LENGTH_PREFIXED:
        writeByte(length);
        while (*text != 0) {
            wchar_t character = *text;
            writeByte(character >> 8);
            writeByte(character);
            text++;
        }
        break;
    case WCHAR_TERMINATED:
        for (int index = 0; index < length; index++) {
            wchar_t character = text[index];
            writeByte(character >> 8);
            writeByte(character);
        }
        writeByte(terminator >> 8);
        writeByte(terminator);
        break;
    case WCHAR_RAW:
        for (int index = 0; index < length; index++) {
            wchar_t character = text[index];
            writeByte(character >> 8);
            writeByte(character);
        }
        break;
    }
}

unsigned char* DataOutputStream::GetData() const
{
    return m_buffer.m_items;
}

void DataOutputStream::clear()
{
    m_buffer.removeAllElements();
}

void DataOutputStream::write(unsigned char* bytes, unsigned int count)
{
    m_buffer.ensureCapacity(m_buffer.size() + count);
    for (unsigned int index = 0; index < count; index++) {
        m_buffer.addElement(bytes[index]);
    }
}
