// CPHManager.mm - Glu's Objective-C front end to the PlayHaven SDK: publisher open/content requests,
// notification badges per placement, and an optional C++ listener for the content callbacks.
#import "CPHManager.h"

#import "AppView.h"

@implementation CPHManager

@synthesize activePlacement = m_activePlacement;

- (void)dealloc
{
    for (PHNotificationView* view in [m_notificationViews allValues]) {
        [view removeFromSuperview];
    }
    [m_token release];
    [m_secret release];
    [m_notificationViews release];
    [m_activePlacement release];
    [super dealloc];
}

- (id)init:(const char*)token :(const char*)secret
{
    self = [super init];
    if (self != nil) {
        m_token = [[NSString alloc] initWithCString:token encoding:NSASCIIStringEncoding];
        m_secret = [[NSString alloc] initWithCString:secret encoding:NSASCIIStringEncoding];
        m_notificationViews = [[NSMutableDictionary alloc] init];
        m_listener = NULL;
        m_showErrorMessage = true;
    }
    return self;
}

- (void)startPublisherOpenRequest
{
    [[PHPublisherOpenRequest requestForApp:m_token secret:m_secret] send];
}

- (void)startPublisherContentRequest:(const char*)placement :(bool)showsOverlayImmediately :(bool)showErrorMessage
{
    // one content request at a time
    if ([self activePlacement] == nil) {
        m_showErrorMessage = showErrorMessage;
        NSString* placementName = [[NSString alloc] initWithCString:placement encoding:NSASCIIStringEncoding];
        [self setActivePlacement:placementName];
        PHPublisherContentRequest* request = [PHPublisherContentRequest requestForApp:m_token
                                                                               secret:m_secret
                                                                            placement:placementName
                                                                             delegate:self];
        [request setShowsOverlayImmediately:showsOverlayImmediately];
        [request send];
        [placementName release];
    }
}

- (void)cancelPublisherContentRequest
{
    [PHPublisherOpenRequest cancelAllRequestsWithDelegate:self];
    [self setActivePlacement:nil];
}

- (bool)isPublisherContentRequestActive:(const char*)placement
{
    if ([self activePlacement] != nil && placement != NULL) {
        NSString* placementName = [NSString stringWithCString:placement encoding:NSASCIIStringEncoding];
        return [[self activePlacement] isEqualToString:placementName];
    }
    return false;
}

- (void)requestWillGetContent:(PHPublisherContentRequest*)request
{
    if (m_listener != NULL) {
        m_listener->OnRequestWillGetContent();
    }
}

- (void)request:(PHPublisherContentRequest*)request contentWillDisplay:(PHContent*)content
{
    if (m_listener != NULL) {
        m_listener->OnContentWillDisplay();
    }
}

- (void)request:(PHPublisherContentRequest*)request contentDidDisplay:(PHContent*)content
{
    // the placement's notifications have been seen
    [[m_notificationViews objectForKey:[request placement]] clear];
    if (m_listener != NULL) {
        m_listener->OnContentDidDisplay();
    }
}

- (void)request:(PHPublisherContentRequest*)request contentDidDismissWithType:(PHPublisherContentDismissType*)type
{
    [self setActivePlacement:nil];
    if (m_listener != NULL) {
        m_listener->OnContentDidDismiss();
    }
}

- (void)request:(PHPublisherContentRequest*)request didFailWithError:(NSError*)error
{
    [self setActivePlacement:nil];
    if (m_listener != NULL) {
        m_listener->OnRequestFailed([[error localizedDescription] UTF8String]);
    } else if (m_showErrorMessage) {
        [self showErrorMessage:[error code]];
    }
}

- (void)request:(PHPublisherContentRequest*)request unlockedReward:(PHReward*)reward
{
}

// both alerts end the button list with an extra nil (as in the original)
- (void)showErrorMessage:(int)errorCode
{
    UIAlertView* alert;
    if (errorCode == NSURLErrorNotConnectedToInternet) {
        alert = [[UIAlertView alloc] initWithTitle:@""
                                           message:@"Network connection is required to use this feature."
                                          delegate:nil
                                 cancelButtonTitle:@"OK"
                                 otherButtonTitles:nil, nil];
    } else {
        alert = [[UIAlertView alloc] initWithTitle:@""
                                           message:@"Sorry this feature is not available at the moment. Try again later."
                                          delegate:nil
                                 cancelButtonTitle:@"OK"
                                 otherButtonTitles:nil, nil];
    }
    [alert show];
    [alert release];
}

- (void)showNotificationView:(const char*)placement :(bool)testMode :(unsigned int)centerX :(unsigned int)centerY
{
    NSString* placementName = [NSString stringWithCString:placement encoding:NSASCIIStringEncoding];
    PHNotificationView* view = [[m_notificationViews objectForKey:placementName] retain];
    if (view == nil) {
        // first use of this placement: create the badge and put it on the game view
        view = [[PHNotificationView alloc] initWithApp:m_token secret:m_secret placement:placementName];
        [m_notificationViews setValue:view forKey:placementName];
        UIWindow* window = [[[UIApplication sharedApplication] windows] objectAtIndex:0];
        for (UIView* subview in [window subviews]) {
            if ([subview isKindOfClass:[AppView class]]) {
                [subview addSubview:view];
                break;
            }
        }
    }
    [view setHidden:NO];
    [view setAutoresizingMask:UIViewAutoresizingFlexibleLeftMargin];
    [view setCenter:CGPointMake(centerX, centerY)];
    // written as "== 0" with the refresh branch first: matches the original branch layout
    if (testMode == 0) {
        [view refresh];
    } else {
        [view test];
    }
    [view release];
}

- (void)hideNotificationView:(const char*)placement
{
    NSString* placementName = [NSString stringWithCString:placement encoding:NSASCIIStringEncoding];
    [[m_notificationViews valueForKey:placementName] setHidden:YES];
}

- (void)registerListener:(CPHListener*)listener
{
    m_listener = listener;
}

@end
