// AppView.h - the application views: AppView (nib view) and AppGLView (CAEAGLLayer-backed view
// that renders the game). Both forward touches to the CApplet as system events.
#import <UIKit/UIKit.h>

@interface AppView : UIView {
    float m_viewScale;      // content scale factor (2 on Retina screens)
    int m_width;            // view size in points (16.16 rounded down)
    int m_height;
}

@end

@interface AppGLView : UIView {
    float m_viewScale;      // content scale factor (2 on Retina screens)
    int m_width;            // view size in points
    int m_height;
}

@end
