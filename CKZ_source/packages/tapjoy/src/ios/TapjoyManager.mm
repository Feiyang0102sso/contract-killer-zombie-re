// TapjoyManager.mm - Glu's Objective-C front end to the Tapjoy SDK.
// Tap points are mirrored locally (NSUserDefaults) so the game can spend them while offline;
// the display ad banner is driven by a small state machine ticked by the game.
#import "TapjoyManager.h"

#include "TapjoyInterface.h"
#include "TapjoyUtil.h"

using com::glu::platform::components::CStrWChar;

/** Display ad banner states (m_adState). */
enum {
    AdState_Check = 0,      // first tick after showAd: look whether an ad is loaded
    AdState_Shown = 1,      // banner visible, counting down to the next refresh
    AdState_Loading = 2,    // refresh requested, waiting for the new ad
    AdState_Failed = 3      // no ad, counting down to a retry
};

/** Banner refresh / retry delays, ms. */
static const int kAdRefreshTime = 60000;
static const int kAdRetryTime = 15000;

/** NSUserDefaults keys of the local tap point mirror. */
#define kLocalLastPointTotalKey         @"GLU_TAPJOY_MANAGER_LOCAL_LAST_POINT_TOTAL_ID"
#define kLocalRemainingPointTotalKey    @"GLU_TAPJOY_MANAGER_LOCAL_REMAINING_POINT_TOTAL_ID"

@implementation TapjoyManager

- (id)init:(const char*)appId :(const char*)secretKey :(bool)enableVideoAds
{
    self = [super init];
    if (self != nil) {
        appDelegate = (AppDelegate*)[[UIApplication sharedApplication] delegate];
        window = [appDelegate window];

        // iPad screens get the large banner
        CGRect screenBounds = [[UIScreen mainScreen] bounds];
        if (screenBounds.size.width == 768 || screenBounds.size.height == 768) {
            m_tapjoyAdsBannerSize = TJC_AD_BANNERSIZE_768X90;
        } else {
            m_tapjoyAdsBannerSize = TJC_AD_BANNERSIZE_320X50;
        }

        m_tapjoyId = [NSString stringWithCString:appId encoding:NSASCIIStringEncoding];
        NSString* secret = [NSString stringWithCString:secretKey encoding:NSASCIIStringEncoding];
        m_tapjoyConnected = 0;
        m_tapjoyOpen = 0;
        m_gotUpdatedPoints = 1;
        m_previousConsumed = 0;
        m_adView = nil;
        m_localLastPointTotal = 0;
        m_localRemainingPointTotal = 0;
        m_tapjoyFirstLaunch = 1;
        if ([self loadLocalPoints]) {
            m_tapjoyFirstLaunch = 0;
        }

        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(tjcConnectSuccess:) name:TJC_CONNECT_SUCCESS object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(tjcConnectFail:) name:TJC_CONNECT_FAILED object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(tjcWebViewClosed:) name:TJC_VIEW_CLOSED_NOTIFICATION object:nil];

        m_tapjoy = [[TapjoyConnect requestTapjoyConnect:m_tapjoyId secretKey:secret] retain];
        [TapjoyConnect setTransitionEffect:0];
        if (enableVideoAds == true) {
            [TapjoyConnect initVideoAdWithDelegate:self];
            [TapjoyConnect setVideoCacheCount:1];
        }

        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(getFeaturedApp:) name:TJC_FEATURED_APP_RESPONSE_NOTIFICATION object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(getUpdatePoints:) name:TJC_TAP_POINTS_RESPONSE_NOTIFICATION object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(getSpentPoints:) name:TJC_SPEND_TAP_POINTS_RESPONSE_NOTIFICATION object:nil];

        // display ad banner: created hidden on top of the key window's root view
        [TapjoyConnect getDisplayAdWithDelegate:self];
        m_adView = [[TapjoyConnect getDisplayAdView] retain];
        [[[[[UIApplication sharedApplication] keyWindow] subviews] objectAtIndex:0] addSubview:m_adView];
        m_showBanner = 0;
        m_adState = AdState_Check;
        [m_adView setHidden:YES];
        m_timeToRefreshAds = 0;
        m_timeToRetryAds = 0;
    }
    return self;
}

- (void)dealloc
{
    [m_adView release];
    [m_tapjoyId release];
    [m_tapjoy release];
    [super dealloc];
}

- (NSString*)getTapjoyID
{
    return m_tapjoyId;
}

- (void)tick:(int)deltaTime
{
    if (m_showBanner == 0) {
        return;
    }
    switch (m_adState) {
    case AdState_Check:
        if ([TapjoyConnect isDisplayAdLoaded]) {
            [m_adView setHidden:NO];
            m_adState = AdState_Shown;
            m_timeToRefreshAds = kAdRefreshTime;
        } else {
            m_adState = AdState_Failed;
            m_timeToRetryAds = kAdRetryTime;
        }
        break;
    case AdState_Shown:
        m_timeToRefreshAds -= deltaTime;
        if (m_timeToRefreshAds <= 0 && m_adView != nil) {
            [TapjoyConnect refreshDisplayAd];
            m_adState = AdState_Loading;
        }
        break;
    case AdState_Loading:
        if ([TapjoyConnect isDisplayAdLoaded]) {
            [m_adView setHidden:NO];
            m_adState = AdState_Shown;
            m_timeToRefreshAds = kAdRefreshTime;
        }
        break;
    case AdState_Failed:
        m_timeToRetryAds -= deltaTime;
        if (m_timeToRetryAds <= 0 && m_adView != nil) {
            [TapjoyConnect refreshDisplayAd];
            m_adState = AdState_Loading;
        }
        break;
    }
}

- (unsigned char)canDisplayInterface
{
    return m_tapjoyConnected;
}

- (unsigned char)interfaceIsOpen
{
    return m_tapjoyOpen;
}

- (void)displayInterface
{
    // present the offer wall from the view controller owning one of the window's views
    UIResponder* viewController = nil;
    UIWindow* mainWindow = [[[UIApplication sharedApplication] windows] objectAtIndex:0];
    for (UIView* subview in [mainWindow subviews]) {
        UIResponder* responder = [subview nextResponder];
        if ([responder isKindOfClass:[UIViewController class]]) {
            viewController = responder;
            break;
        }
    }
    [TapjoyConnect showOffersWithViewController:(UIViewController*)viewController];
    [self tjcWebViewOpen];
}

- (void)updateVideoOrientation
{
    UIDeviceOrientation orientation = [[UIDevice currentDevice] orientation];
    [TapjoyConnect updateViewsWithOrientation:(UIInterfaceOrientation)orientation];
}

- (void)updateBounds:(UIView*)view
{
    int orientation = TapjoyUtil::GetDeviceOrientation();
    CGRect screenBounds = [[UIScreen mainScreen] bounds];
    unsigned int screenWidth = screenBounds.size.width;
    unsigned int screenHeight = screenBounds.size.height;
    if ((orientation == 2 || orientation == 4) && screenHeight > screenWidth) {
        // landscape device: make the screen size landscape too
        unsigned int temp = screenWidth;
        screenWidth = screenHeight;
        screenHeight = temp;
    }

    UIViewController* viewController = [TJCCallsWrapper sharedTJCCallsWrapper];
    if (viewController == nil) {
        id delegate = [[UIApplication sharedApplication] delegate];
        if ([delegate respondsToSelector:@selector(viewController)]) {
            viewController = [delegate viewController];
        }
    }

    // the frame follows the interface orientation (landscape when unknown)
    UIInterfaceOrientation interfaceOrientation = UIInterfaceOrientationLandscapeRight;
    if (viewController != nil) {
        interfaceOrientation = [viewController interfaceOrientation];
    }
    unsigned int frameWidth = screenWidth;
    unsigned int frameHeight = screenHeight;
    if (!UIInterfaceOrientationIsLandscape(interfaceOrientation) && UIInterfaceOrientationIsPortrait(interfaceOrientation)) {
        if (screenWidth > screenHeight) {
            frameWidth = screenHeight;
            frameHeight = screenWidth;
        }
    } else {
        if (screenWidth < screenHeight) {
            frameWidth = screenHeight;
            frameHeight = screenWidth;
        }
    }

    CGAffineTransform transform = CGAffineTransformMakeRotation(0);
    switch (orientation) {
    case 3:
        transform = CGAffineTransformMakeRotation(M_PI);
        break;
    case 2:
        transform = CGAffineTransformMakeRotation(M_PI * 1.5);
        break;
    case 4:
        transform = CGAffineTransformMakeRotation(M_PI_2);
        break;
    }
    [view setTransform:transform];
    [view setFrame:CGRectMake(0, 0, frameWidth, frameHeight)];
}

- (void)removeInterface
{
    [[TJCCallsWrapper sharedTJCCallsWrapper] hideView];
}

- (void)onResume
{
    // ask for the points earned while the game was in the background
    if (m_gotUpdatedPoints != 0) {
        m_gotUpdatedPoints = 0;
        [TapjoyConnect getTapPoints];
    }
}

- (void)tjcConnectSuccess:(NSNotification*)notification
{
    m_tapjoyConnected = 1;
    [TapjoyConnect getTapPoints];
}

- (void)tjcConnectFail:(NSNotification*)notification
{
}

- (void)getUpdatePoints:(NSNotification*)notification
{
    unsigned int points = [[notification object] intValue];
    // points earned since the last update become spendable locally
    if (points > m_localLastPointTotal) {
        m_localRemainingPointTotal += points - m_localLastPointTotal;
    }
    m_serverTapjoyPoints = points;
    if (m_tapjoyFirstLaunch) {
        // points earned before the first launch are not given out again
        m_localRemainingPointTotal = 0;
        m_tapjoyFirstLaunch = 0;
    }
    m_localLastPointTotal = points;
    [self saveLocalPoints];
    m_gotUpdatedPoints = 1;
}

- (void)getSpentPoints:(NSNotification*)notification
{
    m_serverTapjoyPoints = [[notification object] intValue];
}

- (void)queryFeaturedApp
{
    [TapjoyConnect getFeaturedApp];
}

- (BOOL)isIntValue:(NSString*)string
{
    for (unsigned int index = 0; index < [string length]; index++) {
        unichar character = [string characterAtIndex:index];
        if (![[NSCharacterSet decimalDigitCharacterSet] characterIsMember:character]) {
            return NO;
        }
    }
    return YES;
}

- (NSString*)replaceHTMLCodesInString:(NSString*)string
{
    // replaces numeric character references ("&#NNN;") by the character
    if (string == nil) {
        return nil;
    }
    if ([string length] == 0) {
        return string;
    }

    NSArray* components = [string componentsSeparatedByString:@"&#"];
    NSMutableArray* parts = [NSMutableArray arrayWithCapacity:[components count]];
    [parts addObjectsFromArray:components];
    NSString* result = [@"" stringByAppendingString:[parts objectAtIndex:0]];
    [parts removeObjectAtIndex:0];
    if ([parts count] > 0) {
        while ([parts count] > 0) {
            NSString* part = [parts objectAtIndex:0];
            NSRange range = [part rangeOfString:@";"];
            NSString* code = nil;
            if (range.location != NSNotFound) {
                code = [part substringToIndex:range.location];
            }
            if (code != nil && range.location != 0 && [self isIntValue:code]) {
                // "NNN;rest" -> character + "rest"
                range.length = range.location + 1;
                range.location = 0;
                NSString* character = [NSString stringWithFormat:@"%C", [code intValue]];
                result = [result stringByAppendingString:[part stringByReplacingCharactersInRange:range withString:character]];
            } else {
                // not a valid code: keep the text as it was
                result = [result stringByAppendingString:@"&#"];
                result = [result stringByAppendingString:[parts objectAtIndex:0]];
            }
            [parts removeObjectAtIndex:0];
        }
    }
    return [[result retain] autorelease];
}

- (void)getFeaturedApp:(NSNotification*)notification
{
    TJCFeaturedAppModel* featuredApp = [notification object];
    // the previous info is deleted even when no new one arrives (the pointer is left dangling then)
    if (TapjoyInterface::m_pFeaturedAppInfo != NULL) {
        delete TapjoyInterface::m_pFeaturedAppInfo;
    }
    if (featuredApp != nil) {
        TapjoyInterface::FeaturedAppInfo* info = new TapjoyInterface::FeaturedAppInfo;
        [featuredApp setName:[self replaceHTMLCodesInString:[featuredApp name]]];
        const wchar_t* name = (const wchar_t*)[[featuredApp name] cStringUsingEncoding:NSUTF32LittleEndianStringEncoding];
        [featuredApp setDescription:[self replaceHTMLCodesInString:[featuredApp description]]];
        const wchar_t* description = (const wchar_t*)[[featuredApp description] cStringUsingEncoding:NSUTF32LittleEndianStringEncoding];
        info->m_name = CStrWChar(name);
        info->m_cost = CStrWChar([[featuredApp cost] cStringUsingEncoding:NSUTF8StringEncoding]);
        info->m_description = CStrWChar(description);
        info->m_iconURL = CStrWChar([[featuredApp iconURL] cStringUsingEncoding:NSUTF8StringEncoding]);
        info->m_redirectURL = CStrWChar([[featuredApp redirectURL] cStringUsingEncoding:NSUTF8StringEncoding]);
        info->m_amount = [featuredApp amount];
        info->m_maxTimesToDisplayThisApp = [featuredApp maxTimesToDisplayThisApp];
        TapjoyInterface::m_pFeaturedAppInfo = info;
        TapjoyInterface::m_FeaturedAppQueryInProgress = false;
    }
}

- (void)tjcWebViewOpen
{
    m_tapjoyOpen = 1;
}

- (void)tjcWebViewClosed:(NSNotification*)notification
{
    [[TJCCallsWrapper sharedTJCCallsWrapper] dismissModalViewControllerAnimated:YES];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:UIDeviceOrientationDidChangeNotification object:nil];
    m_tapjoyOpen = 0;
    if (m_gotUpdatedPoints) {
        m_gotUpdatedPoints = 0;
        [TapjoyConnect getTapPoints];
    }
}

- (unsigned int)getRemainingTapjoyPoints
{
    return m_localRemainingPointTotal;
}

- (unsigned int)getLastTapjoyPointTotal
{
    return m_localLastPointTotal;
}

- (unsigned char)consumeTapjoyPoints:(unsigned int)points
{
    unsigned char consumed = 0;
    if (points != 0) {
        if (m_localRemainingPointTotal < points) {
            return 0;
        }
        m_localRemainingPointTotal -= points;
        [self saveLocalPoints];
        m_previousConsumed = points;
        consumed = 1;
    }
    return consumed;
}

- (void)saveLocalPoints
{
    [[NSUserDefaults standardUserDefaults] setValue:[NSNumber numberWithInt:m_localLastPointTotal]
                                             forKey:kLocalLastPointTotalKey];
    [[NSUserDefaults standardUserDefaults] setValue:[NSNumber numberWithInt:m_localRemainingPointTotal]
                                             forKey:kLocalRemainingPointTotalKey];
}

- (unsigned char)loadLocalPoints
{
    id lastPointTotal = [[NSUserDefaults standardUserDefaults] objectForKey:kLocalLastPointTotalKey];
    id remainingPointTotal = [[NSUserDefaults standardUserDefaults] objectForKey:kLocalRemainingPointTotalKey];
    if (lastPointTotal != nil && remainingPointTotal != nil) {
        m_localLastPointTotal = [lastPointTotal intValue];
        m_localRemainingPointTotal = [remainingPointTotal intValue];
        return 1;
    }
    return 0;
}

- (unsigned int)getServerTapjoyPoints
{
    return m_serverTapjoyPoints;
}

- (unsigned char)consumeServerTapjoyPoints:(unsigned int)points
{
    unsigned char consumed = 0;
    if (points <= m_serverTapjoyPoints && m_tapjoy != nil) {
        m_serverTapjoyPoints -= points;
        [TapjoyConnect spendTapPoints:points];
        consumed = 1;
    }
    return consumed;
}

- (unsigned int)convertPointsToGameCurrency:(unsigned int)points
{
    return points;
}

- (unsigned int)getPreviousConsumedThisSession
{
    return m_previousConsumed;
}

- (unsigned int)getExchangeRate
{
    return 1;
}

- (void)showAd:(int)alignment :(int)x :(int)y
{
    m_showBanner = 1;
    if (m_adView == nil) {
        return;
    }
    [m_adView setTransform:CGAffineTransformIdentity];
    int adWidth = [m_adView frame].size.width;
    int adHeight = [m_adView frame].size.height;
    UIWindow* keyWindow = [[UIApplication sharedApplication] keyWindow];
    if (x == -1 && y == -1) {
        // no position given: place the banner by the alignment flags
        CGRect bounds = [[[keyWindow subviews] objectAtIndex:0] bounds];
        int verticalAlignment = alignment & 3;
        int horizontalAlignment = alignment & 0xc;
        switch (verticalAlignment) {
        case 3:
            y = bounds.size.height - adHeight;
            break;
        case 0:
            y = (bounds.size.height - adHeight) * 0.5f;
            break;
        case 1:
            y = 0;
            break;
        default:
            // NSAssert records __FILE__/__LINE__: restore the original build path and line (955)
#line 955 "/Applications/buildAgent/work/c426877c688cbe05/packages/tapjoy/src/ios/TapjoyManager.mm"
            NSAssert(NO, @"TapjoyAds position failed");
            break;
        }
        // horizontal placement (the second assertion is on line 970 of the original file)
        switch (horizontalAlignment) {
        case 8:
            x = bounds.size.width - adWidth;
            break;
        case 0:
            x = (bounds.size.width - adWidth) * 0.5f;
            break;
        case 0xc:
            x = 0;
            break;
        default:
            NSAssert(NO, @"TapjoyAds position failed");
            break;
        }
    } else if ([keyWindow respondsToSelector:@selector(contentScaleFactor)]) {
        // the position is given in pixels
        x = x / [[UIScreen mainScreen] scale];
        y = y / [[UIScreen mainScreen] scale];
    }
    [m_adView setFrame:CGRectMake(x, y, adWidth, adHeight)];
}

- (void)hideAd
{
    m_showBanner = 0;
    if (m_adView != nil) {
        m_adState = AdState_Check;
        m_timeToRefreshAds = 0;
        [m_adView setHidden:YES];
    }
}

- (bool)isVisible
{
    return ![m_adView isHidden];
}

- (void)registerVideoCallback:(TapjoyVideoAdCallback)callback :(void*)context
{
    m_videoAdCallback = callback;
    m_videoCallbackPtr = context;
}

- (void)setVideoCacheCount:(unsigned int)count
{
    [TapjoyConnect setVideoCacheCount:count];
}

- (void)didRecieveAd:(UIView*)adView
{
}

- (void)didFailWithMessage:(NSString*)message
{
    if (m_showBanner) {
        m_adState = AdState_Failed;
        m_timeToRetryAds = kAdRetryTime;
    }
}

- (NSString*)adContentSize
{
    return m_tapjoyAdsBannerSize;
}

- (BOOL)shouldRefreshAd
{
    return NO;
}

- (void)videoAdBegan
{
    if (m_videoAdCallback != NULL) {
        m_videoAdCallback(1, m_videoCallbackPtr);
    }
}

- (void)videoAdClosed
{
    if (m_videoAdCallback != NULL) {
        m_videoAdCallback(0, m_videoCallbackPtr);
    }
}

- (void)debugGivePoints:(unsigned int)points
{
    NSLog(@"*********************************************************");
    NSLog(@"********** Debug is on! User got free points! ***********");
    NSLog(@"*********************************************************");
    m_serverTapjoyPoints += points;
    m_localRemainingPointTotal += points;
    [self saveLocalPoints];
}

@end
