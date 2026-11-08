// CStringUtils.mm - NSString <-> CStrWChar conversion (UTF-32 little endian = wchar_t on iOS).
#include "CStringUtils.h"

#include "CStrWCharBuffer.h"

using com::glu::platform::components::CStrWChar;
using com::glu::platform::components::CStrWCharBuffer;

CStrWChar CStringUtils::NSStringToCStrWChar(NSString* string)
{
    int length = [string length];
    CStrWCharBuffer buffer(length);
    buffer.Append((const wchar_t*)[string cStringUsingEncoding:NSUTF32LittleEndianStringEncoding], 0, length);
    return CStrWChar(buffer.GetWCharPtr());
}

NSString* CStringUtils::CStrWCharToNSString(const CStrWChar& string)
{
    return [[NSString alloc] initWithBytes:string.GetWCharPtr()
                                    length:string.GetLength() * sizeof(wchar_t)
                                  encoding:NSUTF32LittleEndianStringEncoding];
}
