// NSString+WCharPtr.h - NSString construction from a wchar_t C string.
#ifndef NSSTRING_WCHARPTR_H
#define NSSTRING_WCHARPTR_H

#import <Foundation/Foundation.h>

@interface NSString (WCharPtr)

/** NSString with the UTF-8 conversion of `text`. Returned retained (+1), as in the original. */
+ (NSString*)stringWithWCharPtr:(const wchar_t*)text;

@end

#endif
