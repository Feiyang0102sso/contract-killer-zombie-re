// NSObject+QueryComponents.m - URL query string <-> dictionary helpers used by PlayHaven.
// A key that appears several times collects its values in an NSMutableArray.
#import "NSObject+QueryComponents.h"

@implementation NSString (QueryComponents)

- (NSString*)stringByDecodingURLFormat
{
    NSString* result = [self stringByReplacingOccurrencesOfString:@"+" withString:@" "];
    result = [result stringByReplacingPercentEscapesUsingEncoding:NSUTF8StringEncoding];
    return result;
}

- (NSString*)stringByEncodingURLFormat
{
    NSString* result = [self stringByReplacingOccurrencesOfString:@" " withString:@"+"];
    result = [result stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding];
    return result;
}

- (NSMutableDictionary*)dictionaryFromQueryComponents
{
    NSMutableDictionary* queryComponents = [NSMutableDictionary dictionary];
    for (NSString* keyValuePairString in [self componentsSeparatedByString:@"&"]) {
        NSArray* keyValuePairArray = [keyValuePairString componentsSeparatedByString:@"="];
        // needs at least a key and a value; extra '=' are ignored
        if ([keyValuePairArray count] < 2) {
            continue;
        }
        NSString* key = [[keyValuePairArray objectAtIndex:0] stringByDecodingURLFormat];
        NSString* value = [[keyValuePairArray objectAtIndex:1] stringByDecodingURLFormat];
        id existing = [queryComponents objectForKey:key];
        if (existing == nil) {
            [queryComponents setObject:value forKey:key];
        } else if (![existing isKindOfClass:[NSMutableArray class]]) {
            // second value: turn the entry into an array
            [queryComponents setObject:[NSMutableArray arrayWithObjects:existing, value, nil] forKey:key];
        } else {
            [existing addObject:value];
        }
    }
    return queryComponents;
}

@end

@implementation NSURL (QueryComponents)

- (NSMutableDictionary*)queryComponents
{
    return [[self query] dictionaryFromQueryComponents];
}

@end

@implementation NSDictionary (QueryComponents)

- (NSString*)stringFromQueryComponents
{
    NSString* result = nil;
    for (NSString* key in [self allKeys]) {
        // the encoded key is also used for the lookup (as in the original)
        key = [key stringByEncodingURLFormat];
        NSArray* allValues = [self objectForKey:key];
        if ([allValues isKindOfClass:[NSArray class]]) {
            for (NSString* value in allValues) {
                value = [[value description] stringByEncodingURLFormat];
                if (result == nil) {
                    result = [NSString stringWithFormat:@"%@=%@", key, value];
                } else {
                    result = [result stringByAppendingFormat:@"&%@=%@", key, value];
                }
            }
        } else {
            NSString* value = [[allValues description] stringByEncodingURLFormat];
            if (result == nil) {
                result = [NSString stringWithFormat:@"%@=%@", key, value];
            } else {
                result = [result stringByAppendingFormat:@"&%@=%@", key, value];
            }
        }
    }
    return result;
}

@end
