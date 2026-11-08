// CGenericTextEntryViewDialog.mm - one line text entry dialog of gServe.
#import "CGenericTextEntryViewDialog.h"
#include <wchar.h>
#include "CNGS_Platform.h"
#include "CUtf.h"

using com::glu::platform::core::CUtf;

/** Layout of the dialog (points). */
static const float MESSAGE_FONT_SIZE = 12.0f;
static const float MESSAGE_MAX_WIDTH = 320.0f;
static const float MIN_CONTENT_WIDTH = 190.0f;
static const float MARGIN = 10.0f;
static const float BUTTON_WIDTH = 95.0f;
static const float BUTTON_HEIGHT = 40.0f;
/** Landscape shift of the dialog (m_offset). */
static const int LANDSCAPE_OFFSET = 80;
/** NSUTF32LittleEndianStringEncoding: wchar_t text. */
static const NSStringEncoding WCHAR_ENCODING = 0x9c000100;

@implementation CGenericTextEntryViewDialog

- (id)initWithMessage:(NSString*)message andFunctor:(CDialogFunctor*)functor andTextFieldSize:(CGSize)size andTextAddress:(wchar_t**)text
{
    [super initWithFrame:CGRectMake(0, 0, 0, 0)];
    m_messageText = [[NSString alloc] initWithString:message];
    m_pCallbackFunctor = functor;
    m_fieldSize = size;
    m_ppText = text;
    return self;
}

- (void)loadView
{
    m_offset = LANDSCAPE_OFFSET;

    // size of the message, at least as wide as the text field and the buttons
    float actualFontSize = MESSAGE_FONT_SIZE;
    UIFont* font = [UIFont systemFontOfSize:MESSAGE_FONT_SIZE];
    CGSize messageSize = [m_messageText sizeWithFont:font
                                         minFontSize:MESSAGE_FONT_SIZE
                                      actualFontSize:&actualFontSize
                                            forWidth:MESSAGE_MAX_WIDTH
                                       lineBreakMode:UILineBreakModeWordWrap];
    if (messageSize.width < m_fieldSize.width) {
        messageSize.width = m_fieldSize.width;
    }
    if (messageSize.width < MIN_CONTENT_WIDTH) {
        messageSize.width = MIN_CONTENT_WIDTH;
    }
    m_width = (int)(messageSize.width + 2 * MARGIN);
    m_height = (int)(messageSize.height * 2 + 2 * MARGIN + (messageSize.height + 5.0f) + BUTTON_HEIGHT);
    [super loadView];

    CGRect screenFrame = [[UIScreen mainScreen] applicationFrame];
    [self setFrame:CGRectMake(m_dialog_box_x, m_dialog_box_y, m_width, m_height)];
    [self layoutIfNeeded];
    [self setBackgroundColor:[UIColor clearColor]];
    [self setTransform:transform];

    // message
    m_messageLabel = [[UILabel alloc] initWithFrame:CGRectMake(MESSAGE_FONT_SIZE, MARGIN, messageSize.width, messageSize.height)];
    [m_messageLabel setText:m_messageText];
    [m_messageLabel setTextAlignment:UITextAlignmentCenter];
    [m_messageLabel setBackgroundColor:[UIColor clearColor]];
    [m_messageLabel setTextColor:[UIColor whiteColor]];
    [m_messageLabel setLineBreakMode:UILineBreakModeWordWrap];
    [self addSubview:m_messageLabel];

    // text field below the message, showing the initial text as placeholder
    NSString* placeholder = [[NSString alloc] initWithBytes:*m_ppText length:wcslen(*m_ppText) * sizeof(wchar_t) encoding:WCHAR_ENCODING];
    int textFieldY = (int)(messageSize.height + 4.0f + MARGIN);
    m_pTextField = [self createStandardTextField:placeholder
                                           frame:CGRectMake(MARGIN, textFieldY, m_fieldSize.width, m_fieldSize.height)
                                        isSecure:NO
                                         isEmail:NO];
    [m_pTextField setAutocapitalizationType:UITextAutocapitalizationTypeNone];
    [self addSubview:m_pTextField];

    // Cancel (tag 0) on the left, Ok (tag 1) on the right
    int buttonY = (int)(textFieldY + (m_fieldSize.height + MARGIN));
    m_pCancelButton = [UIButton buttonWithType:UIButtonTypeRoundedRect];
    [m_pCancelButton setTitle:@"Cancel" forState:UIControlStateNormal];
    [m_pCancelButton setFrame:CGRectMake(MARGIN, buttonY, BUTTON_WIDTH, BUTTON_HEIGHT)];
    [m_pCancelButton addTarget:self action:@selector(buttonCancelPressed) forControlEvents:UIControlEventTouchUpInside];
    [m_pCancelButton setBackgroundImage:m_buttonImage forState:UIControlStateNormal];
    [m_pCancelButton setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    [m_pCancelButton setTag:0];
    [self addSubview:m_pCancelButton];

    int okButtonX = (int)(messageSize.width + MARGIN - BUTTON_WIDTH);
    m_pOkButton = [UIButton buttonWithType:UIButtonTypeRoundedRect];
    [m_pOkButton setTitle:@"Ok" forState:UIControlStateNormal];
    [m_pOkButton setFrame:CGRectMake(okButtonX, buttonY, BUTTON_WIDTH, BUTTON_HEIGHT)];
    [m_pOkButton addTarget:self action:@selector(buttonOkPressed:) forControlEvents:UIControlEventTouchUpInside];
    [m_pOkButton setBackgroundImage:m_buttonImage forState:UIControlStateNormal];
    [m_pOkButton setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    [m_pOkButton setTag:1];
    [self addSubview:m_pOkButton];
}

- (UITextField*)createStandardTextField:(NSString*)placeholder frame:(CGRect)frame isSecure:(BOOL)isSecure isEmail:(BOOL)isEmail
{
    UITextField* textField = [[UITextField alloc] initWithFrame:frame];
    [textField setBorderStyle:UITextBorderStyleRoundedRect];
    // ?: on purpose: the original selects the keyboard type with a conditional move
    [textField setKeyboardType:(isEmail ? UIKeyboardTypeEmailAddress : UIKeyboardTypeDefault)];
    [textField setReturnKeyType:UIReturnKeyDone];
    [textField setAutocorrectionType:UITextAutocorrectionTypeNo];
    [textField setPlaceholder:placeholder];
    [textField setSecureTextEntry:isSecure];
    [textField setDelegate:(id<UITextFieldDelegate>)self];    // The original class does not declare this protocol, so ObjC++ needs a cast
    return textField;
}

- (void)buttonOkPressed:(id)sender
{
    if (m_pCallbackFunctor != NULL) {
        *m_ppText = [self NSStringToWChar:[m_pTextField text]];
        m_pCallbackFunctor->buttonPressed([sender tag]);
    }
    [self onExit];
}

- (void)buttonCancelPressed
{
    if (m_pCallbackFunctor != NULL) {
        m_pCallbackFunctor->buttonPressed(0);
    }
    [self onExit];
}

- (wchar_t*)NSStringToWChar:(NSString*)text
{
    wchar_t* wideText;
    if (text == nil || [text length] == 0) {
        wideText = new wchar_t[1];
        wideText[0] = L'\0';
    } else {
        unsigned int length = [text length];
        wideText = new wchar_t[length + 1];
        CUtf::Utf8ToWcs((const unsigned char*)[text UTF8String], length, wideText, length + 1);
        wideText[length] = L'\0';
    }
    return wideText;
}

- (void)onExit
{
    [self hideKeyboard];
    [self removeFromSuperview];
    if (m_pCallbackFunctor != NULL) {
        delete m_pCallbackFunctor;
    }
    [self release];
}

- (BOOL)textFieldShouldReturn:(UITextField*)textField
{
    [textField resignFirstResponder];
    return YES;
}

- (void)show
{
    UIWindow* window = [[UIApplication sharedApplication] keyWindow];
    if (window == nil) {
        window = [[[UIApplication sharedApplication] windows] objectAtIndex:0];
    }
    [self loadView];
    [window addSubview:self];
}

- (void)dealloc
{
    [super dealloc];
}

- (void)drawRect:(CGRect)rect
{
    [m_backgroundImage drawInRect:rect];
}

- (void)hideKeyboard
{
    [m_pTextField resignFirstResponder];
}

@end
