// main_mm.mm - iOS entry point: runs UIApplicationMain inside an autorelease pool.
#import <UIKit/UIKit.h>

int main(int argc, char* argv[])
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    int result = UIApplicationMain(argc, argv, nil, nil);
    [pool release];
    return result;
}
