// PROVISIONAL (created by b033x): stand-in declarations of the PlayHaven iOS SDK.
// The SDK itself (PHAPIRequest.m, PHPublisherContentRequest.m, PHNotificationView.m, ... in this directory)
// is third-party code compiled into zone A and is not rebuilt here; only the classes/selectors that
// CPHManager.mm calls are declared, with the names the original binary uses.
#ifndef PLAYHAVENSDK_H
#define PLAYHAVENSDK_H

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@class PHContent;
@class PHReward;
@class PHPublisherContentRequest;

/** Reason passed to request:contentDidDismissWithType:. */
typedef NSString PHPublisherContentDismissType;

@interface PHAPIRequest : NSObject
+ (void)cancelAllRequestsWithDelegate:(id)delegate;
- (void)send;
@end

@interface PHPublisherOpenRequest : PHAPIRequest
+ (id)requestForApp:(NSString*)token secret:(NSString*)secret;
@end

// b142: methods and order completed from the original protocol_t metadata (the original SDK header was a newer version)
@class PHPurchase;

@protocol PHPublisherContentRequestDelegate <NSObject>
@optional
- (void)requestWillGetContent:(PHPublisherContentRequest*)request;
- (void)requestDidGetContent:(PHPublisherContentRequest*)request;
- (void)request:(PHPublisherContentRequest*)request contentWillDisplay:(PHContent*)content;
- (void)request:(PHPublisherContentRequest*)request contentDidDisplay:(PHContent*)content;
- (void)requestContentDidDismiss:(PHPublisherContentRequest*)request;
- (void)request:(PHPublisherContentRequest*)request contentDidDismissWithType:(PHPublisherContentDismissType*)type;
- (void)request:(PHPublisherContentRequest*)request didFailWithError:(NSError*)error;
- (void)request:(PHPublisherContentRequest*)request contentDidFailWithError:(NSError*)error;
- (UIImage*)request:(PHPublisherContentRequest*)request closeButtonImageForControlState:(UIControlState)state content:(PHContent*)content;
- (UIColor*)request:(PHPublisherContentRequest*)request borderColorForContent:(PHContent*)content;
- (void)request:(PHPublisherContentRequest*)request unlockedReward:(PHReward*)reward;
- (void)request:(PHPublisherContentRequest*)request makePurchase:(PHPurchase*)purchase;
@end

@interface PHPublisherContentRequest : PHAPIRequest
+ (id)requestForApp:(NSString*)token secret:(NSString*)secret placement:(NSString*)placement delegate:(id)delegate;
- (NSString*)placement;
- (void)setShowsOverlayImmediately:(BOOL)showsOverlayImmediately;
@end

/** Badge view showing the number of pending notifications of a placement. */
@interface PHNotificationView : UIView
- (id)initWithApp:(NSString*)token secret:(NSString*)secret placement:(NSString*)placement;
- (void)clear;
- (void)refresh;
- (void)test;
@end

#endif
