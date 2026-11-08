// CGenericButtonViewDialog.mm - alert view for gServe prompt messages.
#import "CGenericButtonViewDialog.h"
#include "CNGS.h"
#include "CNGSLocalUser.h"
#include "CNGSFromServerMessageQ.h"
#include "CNGSFromServerMessage.h"

@implementation CGenericButtonViewDialog

- (id)initWithMessage:(NSString*)message andPrompt:(CNGSMessagePrompt*)prompt andButtonLabelCancel:(NSString*)cancelLabel andButtonOK:(NSString*)okLabel
{
    m_messageText = [[NSString alloc] initWithString:message];
    m_pPromptMessage = prompt;
    m_buttonLabelCancel = cancelLabel;
    m_buttonLabelOK = okLabel;
    UIAlertView* alert = [[UIAlertView alloc] initWithTitle:nil
                                                    message:message
                                                   delegate:self
                                          cancelButtonTitle:m_buttonLabelCancel
                                          otherButtonTitles:m_buttonLabelOK, nil];
    [alert show];
    [alert release];
    return self;
}

- (void)alertView:(UIAlertView*)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
    if (m_pPromptMessage != NULL) {
        CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
        CNGSMessagePrompt* prompt = m_pPromptMessage;
        CNGSMessageJSONBase* okMessage = prompt->m_okMessage;
        CNGSMessageJSONBase* cancelMessage = prompt->m_cancelMessage;
        user->m_messageQueue->MarkMessageForAck(prompt->m_messageId);
        if (buttonIndex == 0) {
            cancelMessage->Apply();
        } else {
            okMessage->Apply();
        }
        if (prompt != NULL) {
            delete prompt;
        }
    }
}

@end
