// CStdUtil_Cocoa_mm.mm - Unicode case mapping of single characters through NSString.
#include "CStdUtil_Cocoa.h"

wchar_t CStdUtil_Cocoa::ToWUpper(wchar_t character)
{
    unichar utf16 = (unichar)character;
    NSString* string = [NSString stringWithCharacters:&utf16 length:1];
    if (string != nil) {
        const wchar_t* converted = (const wchar_t*)[[string uppercaseString] cStringUsingEncoding:NSUTF32StringEncoding];
        if (converted != NULL) {
            character = converted[0];
        }
    }
    return character;
}

wchar_t CStdUtil_Cocoa::ToWLower(wchar_t character)
{
    unichar utf16 = (unichar)character;
    NSString* string = [NSString stringWithCharacters:&utf16 length:1];
    if (string != nil) {
        const wchar_t* converted = (const wchar_t*)[[string lowercaseString] cStringUsingEncoding:NSUTF32StringEncoding];
        if (converted != NULL) {
            character = converted[0];
        }
    }
    return character;
}
