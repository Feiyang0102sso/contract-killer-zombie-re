// FileLogger.h - debug logger that mirrors NSLog output into Documents/memory_report.txt.
#import <Foundation/Foundation.h>

@interface FileLogger : NSObject
{
    NSFileHandle* logFile;      // opened for writing, positioned at the end of the report file
}

/** Process-wide logger, created on first use. */
+ (FileLogger*) sharedInstance;

/** NSLog the formatted message and append it (plus a newline) to the report file. */
- (void) log: (NSString*) format, ...;

@end
