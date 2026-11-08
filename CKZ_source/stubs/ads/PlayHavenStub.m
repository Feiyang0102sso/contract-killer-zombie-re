// b107 STUB (not original code): empty PlayHaven SDK for the first link / play test. See README.md.
// Requests are never created or sent; the notification badge is an empty view.
#import "PlayHavenSDK.h"

@implementation PHAPIRequest

+ (void)cancelAllRequestsWithDelegate:(id)delegate
{
}

- (void)send
{
}

@end

@implementation PHPublisherOpenRequest

+ (id)requestForApp:(NSString*)token secret:(NSString*)secret
{
    return nil;
}

@end

@implementation PHPublisherContentRequest

+ (id)requestForApp:(NSString*)token secret:(NSString*)secret placement:(NSString*)placement delegate:(id)delegate
{
    return nil;
}

- (NSString*)placement
{
    return nil;
}

- (void)setShowsOverlayImmediately:(BOOL)showsOverlayImmediately
{
}

@end

@implementation PHNotificationView

/** CPHManager adds the badge as a subview, so return an empty zero-size view instead of nil. */
- (id)initWithApp:(NSString*)token secret:(NSString*)secret placement:(NSString*)placement
{
    self = [super initWithFrame:CGRectZero];
    return self;
}

- (void)clear
{
}

- (void)refresh
{
}

- (void)test
{
}

@end
