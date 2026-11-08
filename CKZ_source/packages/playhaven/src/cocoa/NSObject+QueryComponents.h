// NSObject+QueryComponents.h - URL query string <-> dictionary helpers used by PlayHaven (created by b033x).
#import <Foundation/Foundation.h>

@interface NSString (QueryComponents)
- (NSString*)stringByDecodingURLFormat;
- (NSString*)stringByEncodingURLFormat;
- (NSMutableDictionary*)dictionaryFromQueryComponents;
@end

@interface NSURL (QueryComponents)
- (NSMutableDictionary*)queryComponents;
@end

@interface NSDictionary (QueryComponents)
- (NSString*)stringFromQueryComponents;
@end
