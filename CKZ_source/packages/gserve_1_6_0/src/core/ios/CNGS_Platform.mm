// CNGS_Platform.mm - iOS services of gServe.
#import <UIKit/UIKit.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <stdlib.h>
#include "CNGS_Platform.h"
#include "CVector2d.h"
#include "CGameCenterInterface.h"
#include "CFacebookInterface.h"
#import "CGenericButtonViewDialog.h"
#import "CGenericTextEntryViewDialog.h"

// isGameCenterSupported() uses the C library heap for the sysctl buffer, as Apple's sample does.
#undef malloc
#undef free

using com::glu::platform::components::CStrWChar;
using com::glu::platform::core::CVector;
using com::glu::platform::math::CVector2d;

/** NSUTF32LittleEndianStringEncoding: wchar_t text. */
static const NSStringEncoding WCHAR_ENCODING = 0x9c000100;

CNGS_Platform::CNGS_Platform()
    : CSingleton(ClassId)
{
    init();
}

void CNGS_Platform::init()
{
    m_gameCenterInterface = new CGameCenterInterface();
    m_facebookInterface = new CFacebookInterface();
    m_gameCenterSupported = isGameCenterSupported();
    m_reserved10 = 0;
}

CNGS_Platform::~CNGS_Platform()
{
    destroy();
}

void CNGS_Platform::destroy()
{
    if (m_gameCenterInterface != NULL) {
        delete m_gameCenterInterface;
        m_gameCenterInterface = NULL;
    }
    if (m_facebookInterface != NULL) {
        delete m_facebookInterface;
        m_facebookInterface = NULL;
    }
}

void CNGS_Platform::showPromptMessageDialog(CStrWChar& message, CNGSMessagePrompt* prompt,
                                            CVector<const CStrWChar*>* buttonLabels, unsigned char hasButtons)
{
    NSString* text = [[NSString alloc] initWithBytes:message.GetWCharPtr()
                                              length:message.GetLength() * sizeof(wchar_t)
                                            encoding:WCHAR_ENCODING];
    [NSMutableArray array];
    // the cancel label stays unset without labels, as in the original
    NSString* cancelLabel;
    NSString* okLabel = nil;
    if (buttonLabels != NULL) {
        cancelLabel = [[NSString alloc] initWithBytes:(*buttonLabels)[0]->GetWCharPtr()
                                               length:(*buttonLabels)[0]->GetLength() * sizeof(wchar_t)
                                             encoding:WCHAR_ENCODING];
        okLabel = [[NSString alloc] initWithBytes:(*buttonLabels)[1]->GetWCharPtr()
                                           length:(*buttonLabels)[1]->GetLength() * sizeof(wchar_t)
                                         encoding:WCHAR_ENCODING];
    }
    [[CGenericButtonViewDialog alloc] initWithMessage:text andPrompt:prompt andButtonLabelCancel:cancelLabel andButtonOK:okLabel];
}

void CNGS_Platform::showGenericTextEntryDialog(CStrWChar& message, CDialogFunctor* functor,
                                               const CVector2d& textFieldSize, wchar_t** text)
{
    // CVector2d and CGSize have the same layout (two floats); the size is passed as is
    NSString* messageText = [[NSString alloc] initWithBytes:message.GetWCharPtr()
                                                     length:message.GetLength() * sizeof(wchar_t)
                                                   encoding:WCHAR_ENCODING];
    CGenericTextEntryViewDialog* dialog = [[CGenericTextEntryViewDialog alloc] initWithMessage:messageText
                                                                                    andFunctor:functor
                                                                              andTextFieldSize:*(const CGSize*)&textFieldSize
                                                                                andTextAddress:text];
    [dialog show];
}

void CNGS_Platform::showGenericAlertPopup(const CStrWChar& message)
{
    NSString* text = [[[NSString alloc] initWithBytes:message.GetWCharPtr()
                                               length:message.GetLength() * sizeof(wchar_t)
                                             encoding:WCHAR_ENCODING] autorelease];
    // the application delegate queues alerts when it can, otherwise show one directly
    if ([[[UIApplication sharedApplication] delegate] respondsToSelector:@selector(queueAlert:)]) {
        [[[UIApplication sharedApplication] delegate] performSelector:@selector(queueAlert:) withObject:text];
    } else {
        UIAlertView* alert = [[UIAlertView alloc] initWithTitle:nil message:text delegate:nil cancelButtonTitle:@"OK" otherButtonTitles:nil];
        [alert show];
        [alert release];
    }
}

unsigned char CNGS_Platform::isGameCenterSupported()
{
    // Game Center classes exist
    bool hasGameCenterClass = (NSClassFromString(@"GKLocalPlayer")) != nil;

    // iOS 4.1 or later
    NSString* requiredVersion = @"4.1";
    NSString* systemVersion = [[UIDevice currentDevice] systemVersion];
    bool osVersionSupported = ([systemVersion compare:requiredVersion options:NSNumericSearch] != NSOrderedAscending);

    // not an iPhone 3G
    size_t size;
    sysctlbyname("hw.machine", NULL, &size, NULL, 0);
    char* machine = (char*)malloc(size);
    sysctlbyname("hw.machine", machine, &size, NULL, 0);
    NSString* platform = [NSString stringWithCString:machine encoding:NSASCIIStringEncoding];
    free(machine);
    bool isUnsupportedDevice = [platform isEqualToString:@"iPhone1,2"];

    return hasGameCenterClass && osVersionSupported && !isUnsupportedDevice;
}

CNGSSocialInterface* CNGS_Platform::getSocialInterface(int type)
{
    CNGSSocialInterface* socialInterface = NULL;
    if (type == SOCIAL_INTERFACE_GAME_CENTER) {
        if (m_gameCenterSupported) {
            socialInterface = m_gameCenterInterface;
        }
    } else if (type == SOCIAL_INTERFACE_FACEBOOK) {
        socialInterface = m_facebookInterface;
    }
    return socialInterface;
}
