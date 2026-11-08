// BundleInterface.mm - reads Info.plist values of the main bundle into CStrWChar.
#include "BundleInterface.h"

using com::glu::platform::components::CStrWChar;

void BundleInterface::BundleIdentifierString(CStrWChar& identifier)
{
    NSBundle* bundle = [NSBundle mainBundle];
    if (bundle != nil) {
        NSString* value = [bundle objectForInfoDictionaryKey:@"CFBundleIdentifier"];
        identifier = [value UTF8String];
    }
}

void BundleInterface::BundleVersionString(CStrWChar& version)
{
    NSBundle* bundle = [NSBundle mainBundle];
    if (bundle != nil) {
        NSString* value = [bundle objectForInfoDictionaryKey:@"CFBundleVersion"];
        version = [value UTF8String];
    }
}
