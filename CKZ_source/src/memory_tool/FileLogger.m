// FileLogger.m - debug logger that mirrors NSLog output into Documents/memory_report.txt.
#import "FileLogger.h"

@implementation FileLogger

- (void) dealloc
{
    [logFile release];
    logFile = nil;
    [super dealloc];
}

- (id) init
{
    // The original compares the result with self instead of assigning it.
    if ([super init] == self) {
        NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
        NSString* documents = [paths objectAtIndex: 0];
        NSString* filePath = [documents stringByAppendingPathComponent: @"memory_report.txt"];
        NSFileManager* fileManager = [NSFileManager defaultManager];
        if (![fileManager fileExistsAtPath: filePath]) {
            [fileManager createFileAtPath: filePath contents: nil attributes: nil];
        }
        logFile = [[NSFileHandle fileHandleForWritingAtPath: filePath] retain];
        [logFile seekToEndOfFile];
        [self log: [NSString stringWithUTF8String: "\n+++++++++++++++++++++\nNEW SESSION STARTED\n+++++++++++++++++++++\n"]];
    }
    return self;
}

- (void) log: (NSString*) format, ...
{
    va_list arguments;
    va_start(arguments, format);
    NSString* message = [[NSString alloc] initWithFormat: format arguments: arguments];
    va_end(arguments);
    NSLog(message);
    [logFile writeData: [[message stringByAppendingString: @"\n"] dataUsingEncoding: NSUTF8StringEncoding]];
    [logFile synchronizeFile];
    [message release];
}

+ (FileLogger*) sharedInstance
{
    static FileLogger* instance;
    if (instance == nil) {
        instance = [[FileLogger alloc] init];
    }
    return instance;
}

@end
