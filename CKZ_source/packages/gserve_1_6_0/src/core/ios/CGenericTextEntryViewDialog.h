// CGenericTextEntryViewDialog.h - gServe dialog asking for one line of text (message, text field,
// Cancel / Ok buttons). The entered text is written to *m_ppText and m_pCallbackFunctor is told.
#ifndef CGENERICTEXTENTRYVIEWDIALOG_H
#define CGENERICTEXTENTRYVIEWDIALOG_H

#import <UIKit/UIKit.h>
#import "CNGSView.h"

class CDialogFunctor;

// b142: the original class_ro protocol list is empty; the original does not declare <UITextFieldDelegate>
@interface CGenericTextEntryViewDialog : CNGSView {
    NSString* m_messageText;
    UILabel* m_messageLabel;
    UIButton* m_pOkButton;
    UIButton* m_pCancelButton;
    UITextField* m_pTextField;
    CGSize m_fieldSize;
    CDialogFunctor* m_pCallbackFunctor;     // owned, deleted by onExit
    wchar_t** m_ppText;                     // in: placeholder text, out: entered text (malloc'd)
}

- (id)initWithMessage:(NSString*)message andFunctor:(CDialogFunctor*)functor andTextFieldSize:(CGSize)size andTextAddress:(wchar_t**)text;
- (void)loadView;
- (UITextField*)createStandardTextField:(NSString*)placeholder frame:(CGRect)frame isSecure:(BOOL)isSecure isEmail:(BOOL)isEmail;
- (void)buttonOkPressed:(id)sender;
- (void)buttonCancelPressed;
- (wchar_t*)NSStringToWChar:(NSString*)text;
- (void)onExit;
- (BOOL)textFieldShouldReturn:(UITextField*)textField;
- (void)show;
- (void)dealloc;
- (void)drawRect:(CGRect)rect;
- (void)hideKeyboard;

@end

#endif
