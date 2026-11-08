// CNGSView.h - base view of the gServe dialogs: background images, a box centred on the screen and
// the rotation that follows the device orientation.
#ifndef CNGSVIEW_H
#define CNGSVIEW_H

#import <UIKit/UIKit.h>

@interface CNGSView : UIView {
    int m_dialog_box_x;                 // left edge of the dialog box
    int m_dialog_box_y;                 // top edge of the dialog box
    int m_width;                        // dialog size (320 x 155 by default)
    int m_height;
    int m_offset;                       // horizontal shift applied in landscape
    UIImage* m_backgroundImage;
    UIImage* m_buttonImage;
    CGAffineTransform transform;        // rotation for the current device orientation
}

- (void)loadView;

@end

#endif
