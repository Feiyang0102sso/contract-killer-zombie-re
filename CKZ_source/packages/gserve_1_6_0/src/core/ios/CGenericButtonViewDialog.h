// CGenericButtonViewDialog.h - UIAlertView showing a server prompt message with two answers;
// the chosen answer's message is applied and the prompt is acknowledged and deleted.
#ifndef CGENERICBUTTONVIEWDIALOG_H
#define CGENERICBUTTONVIEWDIALOG_H

#import <UIKit/UIKit.h>

class CNGSMessagePrompt;

// b142: the original class_ro protocol list is empty; the original does not declare <UIAlertViewDelegate>
@interface CGenericButtonViewDialog : NSObject {
    NSString* m_messageText;
    UILabel* m_messageLabel;
    NSString* m_buttonLabelCancel;
    NSString* m_buttonLabelOK;
    CNGSMessagePrompt* m_pPromptMessage;    // owned, deleted when a button is chosen
}

- (id)initWithMessage:(NSString*)message andPrompt:(CNGSMessagePrompt*)prompt andButtonLabelCancel:(NSString*)cancelLabel andButtonOK:(NSString*)okLabel;
- (void)alertView:(UIAlertView*)alertView clickedButtonAtIndex:(NSInteger)buttonIndex;

@end

#endif
