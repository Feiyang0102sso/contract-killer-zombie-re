// NSString+WCharPtr.mm - wchar_t -> UTF-8 -> NSString.
#include "NSString+WCharPtr.h"
#include <wchar.h>
#include "CUtf.h"

using com::glu::platform::core::CUtf;

@implementation NSString (WCharPtr)

+ (NSString*)stringWithWCharPtr:(const wchar_t*)text
{
    int length = wcslen(text);
    char* utf8 = new char[length + 1];
    int written = CUtf::WcsToUtf8(text, length, (unsigned char*)utf8, length + 1);
    utf8[written] = 0;
    NSString* string = [NSString stringWithUTF8String:utf8];
    if (utf8 != NULL) {
        delete[] utf8;
    }
    // the original returns the convenience string retained
    return [string retain];
}

@end
