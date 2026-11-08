// Original Objective-C bridge at 0x1825d4; argument conversions use ASCII encoding (value 1).
#include "IOSInterface.h"
#import <UIKit/UIKit.h>

@interface NSObject (CKZNativeMessageDeclaration)
- (void)ShowMessage:(NSString*)message Title:(NSString*)title;
@end

void IOSInterface::showMessage(const char* title, const char* message)
{
    id delegate = [[UIApplication sharedApplication] delegate];
    NSString* messageText = [NSString stringWithCString:message encoding:NSASCIIStringEncoding];
    NSString* titleText = [NSString stringWithCString:title encoding:NSASCIIStringEncoding];
    [delegate ShowMessage:messageText Title:titleText];
}
