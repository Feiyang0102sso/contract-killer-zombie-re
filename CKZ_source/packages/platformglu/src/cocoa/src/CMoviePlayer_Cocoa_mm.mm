// CMoviePlayer_Cocoa_mm.mm - full-screen movies through MPMoviePlayerController, with an optional
// separate sound track (AVAudioPlayer) and a transparent overlay view that turns touches into "skip".
#include "CMoviePlayer.h"

#import <MediaPlayer/MediaPlayer.h>
#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>

#include "CApplet.h"
#include "CApp.h"
#include "CHash.h"
#include "ICMediaPlayer.h"
#include "CMediaPlayer_Cocoa.h"

using namespace com::glu::platform::components;

// ---------------------------------------------------------------------------------------------
// OverlayView: transparent view above the movie, posts "OverlayViewTouched" on a new touch
// ---------------------------------------------------------------------------------------------

@interface OverlayView : UIView {
}
@end

@implementation OverlayView

- (void)touchesBegan:(NSSet*)touches withEvent:(UIEvent*)event
{
    if ([[touches anyObject] phase] == UITouchPhaseBegan) {
        [[NSNotificationCenter defaultCenter] postNotificationName:@"OverlayViewTouched" object:nil];
    }
}

@end

// ---------------------------------------------------------------------------------------------
// GluMovieController: owns the movie player and the sound track of one movie
// ---------------------------------------------------------------------------------------------

@interface GluMovieController : NSObject {
    MPMoviePlayerController* movie;     // 0x04
    AVAudioPlayer* soundTrack;          // 0x08 nil without a separate sound track
    OverlayView* overlay;               // 0x0c retained by the view hierarchy only
    BOOL playing;                       // 0x10
    BOOL preparing;                     // 0x11 waiting for "ready to play"
    BOOL touchSkippable;                // 0x12 a touch stops the movie
    CGAffineTransform rotation;         // 0x14 transform of the movie view
    MPMovieScalingMode scaling;         // 0x2c
    unsigned int audioDelay;            // 0x30 milliseconds between movie and sound track start
}

// b142: the original property names are exactly isPlaying / isPreparingToPlay (attribute string Tc,R,N,Vplaying, no custom G getter)
@property (nonatomic, readonly) BOOL isPlaying;
@property (nonatomic, readonly) BOOL isPreparingToPlay;
@property (nonatomic) BOOL touchSkippable;
@property (nonatomic) CGAffineTransform rotation;
@property (nonatomic) MPMovieScalingMode scaling;
@property (nonatomic) unsigned int audioDelay;

- (BOOL)playMovieAtURL:(NSURL*)movieURL withAudio:(NSURL*)audioURL;
- (void)playMovie;
- (void)playAudio;
- (void)playAudioDelayed:(NSTimer*)timer;
- (void)stop;
- (void)pause;
- (void)movieSkipped:(NSNotification*)notification;
- (void)keyWindowChanged:(NSNotification*)notification;
- (void)finishPlayingAndCleanup;
- (void)readyToPlayNotification:(NSNotification*)notification;
- (void)movieFinishedNotification:(NSNotification*)notification;

@end

@implementation GluMovieController

- (void)setRotation:(CGAffineTransform)transform
{
    rotation = transform;
    if (playing || preparing) {
        if ([movie respondsToSelector:@selector(view)]) {
            [[movie view] setTransform:rotation];
        }
    }
}

- (void)setScaling:(MPMovieScalingMode)mode
{
    scaling = mode;
    if (playing || preparing) {
        [movie setScalingMode:scaling];
    }
}

- (id)init
{
    self = [super init];
    if (self != nil) {
        // iOS 3.2+ reports the load state, older players only "preload finished"
        if ([MPMoviePlayerController instancesRespondToSelector:@selector(loadState)]) {
            [[NSNotificationCenter defaultCenter] addObserver:self
                                                     selector:@selector(readyToPlayNotification:)
                                                         name:MPMoviePlayerLoadStateDidChangeNotification
                                                       object:nil];
        } else {
            [[NSNotificationCenter defaultCenter] addObserver:self
                                                     selector:@selector(readyToPlayNotification:)
                                                         name:MPMoviePlayerContentPreloadDidFinishNotification
                                                       object:nil];
            [[NSNotificationCenter defaultCenter] addObserver:self
                                                     selector:@selector(keyWindowChanged:)
                                                         name:UIWindowDidBecomeKeyNotification
                                                       object:nil];
        }
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(movieFinishedNotification:)
                                                     name:MPMoviePlayerPlaybackDidFinishNotification
                                                   object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(movieSkipped:)
                                                     name:@"OverlayViewTouched"
                                                   object:nil];

        rotation = CGAffineTransformIdentity;
        scaling = MPMovieScalingModeAspectFit;
        touchSkippable = YES;
        playing = NO;
        preparing = NO;
    }
    return self;
}

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [movie release];
    [soundTrack release];
    [super dealloc];
}

- (BOOL)playMovieAtURL:(NSURL*)movieURL withAudio:(NSURL*)audioURL
{
    if (playing || preparing || movieURL == nil) {
        return YES;
    }

    // The movie needs the audio hardware: release the game's sound engine meanwhile
    CMediaPlayer_Cocoa* mediaPlayer = (CMediaPlayer_Cocoa*)ICMediaPlayer::GetInstance();
    mediaPlayer->PublicDestroySoundEngine();

    movie = [[MPMoviePlayerController alloc] initWithContentURL:movieURL];
    [movie setScalingMode:scaling];

    if ([movie respondsToSelector:@selector(view)]) {
        // iOS 3.2+: embed the movie view into the key window ourselves
        [movie setControlStyle:MPMovieControlStyleNone];
        [[movie view] setTransform:[self rotation]];

        UIWindow* keyWindow = [[UIApplication sharedApplication] keyWindow];
        [[movie view] setFrame:[[[keyWindow subviews] objectAtIndex:0] bounds]];
        [[[keyWindow subviews] objectAtIndex:0] addSubview:[movie view]];

        overlay = [[OverlayView alloc] initWithFrame:[[movie view] bounds]];
        overlay.backgroundColor = [UIColor clearColor];
        [[movie view] addSubview:overlay];
        [overlay release];
    } else {
        [movie setMovieControlMode:MPMovieControlModeHidden];
    }

    if (audioURL != nil) {
        NSError* error = nil;
        soundTrack = [[AVAudioPlayer alloc] initWithContentsOfURL:audioURL error:&error];
        if (error != nil) {
            [soundTrack release];
            soundTrack = nil;
        } else {
            UInt32 category = kAudioSessionCategory_AmbientSound;
            AudioSessionSetProperty(kAudioSessionProperty_AudioCategory, sizeof(category), &category);
            [soundTrack setNumberOfLoops:0];
            [soundTrack prepareToPlay];
        }
    }

    preparing = YES;
    return YES;
}

- (void)playMovie
{
    if (movie != nil) {
        [movie play];
        playing = YES;
    }
}

- (void)playAudio
{
    [soundTrack play];
}

- (void)playAudioDelayed:(NSTimer*)timer
{
    [soundTrack play];
}

- (void)stop
{
    if (movie != nil && (playing || preparing)) {
        [movie stop];
        [soundTrack stop];
        [self finishPlayingAndCleanup];
    }
}

- (void)pause
{
    if (movie != nil && playing) {
        [movie pause];
        [soundTrack pause];
    }
}

- (void)movieSkipped:(NSNotification*)notification
{
    if (touchSkippable && playing) {
        [self stop];
    }
}

- (void)keyWindowChanged:(NSNotification*)notification
{
    // Old players open their own window: put the touch overlay on top of it
    NSArray* windows = [[UIApplication sharedApplication] windows];
    if ([windows count] >= 2) {
        UIWindow* movieWindow = [windows objectAtIndex:1];
        overlay = [[OverlayView alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
        overlay.backgroundColor = [UIColor clearColor];
        [movieWindow addSubview:overlay];
        [overlay release];
    }
}

- (void)finishPlayingAndCleanup
{
    if ([movie respondsToSelector:@selector(view)]) {
        [[movie view] removeFromSuperview];
    }
    playing = NO;
    preparing = NO;
    [movie release];
    movie = nil;
    [soundTrack release];
    soundTrack = nil;

    // Give the audio hardware back to the game
    CMediaPlayer_Cocoa* mediaPlayer = (CMediaPlayer_Cocoa*)ICMediaPlayer::GetInstance();
    mediaPlayer->PublicInitializeSoundEngine();
}

- (void)readyToPlayNotification:(NSNotification*)notification
{
    if ([notification object] == movie && preparing) {
        preparing = NO;
        [self playMovie];
        if (audioDelay == 0) {
            [self playAudio];
        } else {
            [NSTimer scheduledTimerWithTimeInterval:(float)audioDelay / 1000.0f
                                             target:self
                                           selector:@selector(playAudioDelayed:)
                                           userInfo:nil
                                            repeats:NO];
        }
    }
}

- (void)movieFinishedNotification:(NSNotification*)notification
{
    if (movie != nil && [notification object] == movie && (playing || preparing)) {
        [self finishPlayingAndCleanup];
    }
}

@synthesize isPlaying = playing;
@synthesize isPreparingToPlay = preparing;
@synthesize touchSkippable;
@synthesize rotation;
@synthesize scaling;
@synthesize audioDelay;

@end

// ---------------------------------------------------------------------------------------------
// CMovieEvent_Cocoa: one movie driven through a GluMovieController
// ---------------------------------------------------------------------------------------------

class CMovieEvent_Cocoa : public CMovieEvent {
public:
    CMovieEvent_Cocoa()
    {
        m_controller = nil;
        m_lastError = 0;
    }

    virtual ~CMovieEvent_Cocoa();

    virtual void SetVolume(unsigned char volume);
    virtual unsigned char Initialize();
    virtual void Destroy();
    virtual unsigned char Play();
    virtual unsigned char Stop();
    virtual unsigned char Pause();
    virtual unsigned char Resume();
    virtual unsigned char HandleUpdate(int elapsed);
    virtual int GetLastErr();
    // Source / destination rectangles are not supported on iOS
    virtual unsigned char SetSrcRectX() { return 0; }
    virtual unsigned char SetSrcRectY() { return 0; }
    virtual unsigned char SetSrcRectDX() { return 0; }
    virtual unsigned char SetSrcRectDY() { return 0; }
    virtual unsigned char SetDestRectX() { return 0; }
    virtual unsigned char SetDestRectY() { return 0; }
    virtual unsigned char SetDestRectDX() { return 0; }
    virtual unsigned char SetDestRectDY() { return 0; }
    virtual unsigned char SetTouchSkippable();
    virtual unsigned char SetAudioDelay();
    virtual unsigned char SetScaling();
    virtual unsigned char SetRotation();

private:
    GluMovieController* m_controller;   // 0x70
    int m_lastError;                    // 0x74 0 or -1
};

CMovieEvent* CMovieEvent::CreateInstance()
{
    return new CMovieEvent_Cocoa();
}

CMovieEvent_Cocoa::~CMovieEvent_Cocoa()
{
    if (m_controller != nil) {
        [m_controller release];
        m_controller = nil;
    }
}

void CMovieEvent_Cocoa::SetVolume(unsigned char volume)
{
    CMovieEvent::SetVolume(volume);
}

unsigned char CMovieEvent_Cocoa::Initialize()
{
    Destroy();
    m_controller = [[GluMovieController alloc] init];
    return m_controller != nil;
}

void CMovieEvent_Cocoa::Destroy()
{
    if (m_controller != nil) {
        [m_controller release];
        m_controller = nil;
    }
}

unsigned char CMovieEvent_Cocoa::Play()
{
    if (m_controller != nil && m_fileName.GetWCharPtr() != NULL) {
        NSString* moviePath = [[NSString alloc] initWithBytes:m_fileName.GetWCharPtr()
                                                       length:m_fileName.GetLength() * sizeof(wchar_t)
                                                     encoding:NSUTF32LittleEndianStringEncoding];
        NSURL* movieURL = [NSURL fileURLWithPath:moviePath isDirectory:NO];
        [moviePath release];

        NSURL* audioURL = nil;
        if (m_audioFileName.GetWCharPtr() != NULL) {
            NSString* audioPath = [[NSString alloc] initWithBytes:m_audioFileName.GetWCharPtr()
                                                           length:m_audioFileName.GetLength() * sizeof(wchar_t)
                                                         encoding:NSUTF32LittleEndianStringEncoding];
            audioURL = [NSURL fileURLWithPath:audioPath isDirectory:NO];
            [audioPath release];
        }

        if ([m_controller playMovieAtURL:movieURL withAudio:audioURL]) {
            m_state = 1;
            m_lastError = 0;
            return 1;
        }
    }
    m_lastError = -1;
    return 0;
}

unsigned char CMovieEvent_Cocoa::Stop()
{
    if (m_controller == nil) {
        m_lastError = -1;
        return 0;
    }
    if ([m_controller isPlaying] || [m_controller isPreparingToPlay]) {
        [m_controller stop];
    }
    m_state = StateFinished;
    return m_lastError == 0;
}

unsigned char CMovieEvent_Cocoa::Pause()
{
    if (m_controller == nil) {
        m_lastError = -1;
    } else if ([m_controller isPlaying]) {
        [m_controller pause];
        m_state = 2;
    }
    return m_lastError == 0;
}

unsigned char CMovieEvent_Cocoa::Resume()
{
    // A paused movie cannot be resumed on iOS: it is stopped
    return Stop();
}

unsigned char CMovieEvent_Cocoa::HandleUpdate(int elapsed)
{
    // Playing state but the controller is idle: the movie ended on its own
    if (m_state == 1 && ![m_controller isPreparingToPlay] && ![m_controller isPlaying]) {
        Stop();
    }
    return 1;
}

int CMovieEvent_Cocoa::GetLastErr()
{
    return m_lastError;
}

unsigned char CMovieEvent_Cocoa::SetTouchSkippable()
{
    [m_controller setTouchSkippable:m_touchSkippable];
    return m_controller != nil;
}

unsigned char CMovieEvent_Cocoa::SetAudioDelay()
{
    [m_controller setAudioDelay:m_audioDelay];
    return m_controller != nil;
}

unsigned char CMovieEvent_Cocoa::SetScaling()
{
    MPMovieScalingMode mode = (MPMovieScalingMode)m_scaling;
    if (m_scaling == 1) {
        mode = MPMovieScalingModeAspectFit;
    }
    [m_controller setScaling:mode];
    return m_controller != nil;
}

unsigned char CMovieEvent_Cocoa::SetRotation()
{
    // Degrees to radians
    [m_controller setRotation:CGAffineTransformMakeRotation((float)((unsigned int)m_rotation * M_PI / 180.0))];
    return m_controller != nil;
}

// ---------------------------------------------------------------------------------------------
// CMoviePlayer_Cocoa: the portable player; only touch-skip and audio delay are settable
// ---------------------------------------------------------------------------------------------

class CMoviePlayer_Cocoa : public CMoviePlayer {
public:
    CMoviePlayer_Cocoa()
    {
    }

    virtual ~CMoviePlayer_Cocoa();

    virtual unsigned char CanSetProperty(MovieProperty property);
    virtual unsigned char CanGetProperty(MovieProperty property);
};

ICMoviePlayer* ICMoviePlayer::CreateInstance()
{
    return new CMoviePlayer_Cocoa();
}

CMoviePlayer_Cocoa::~CMoviePlayer_Cocoa()
{
}

unsigned char CMoviePlayer_Cocoa::CanSetProperty(MovieProperty property)
{
    switch (property) {
    case TouchSkippable:
    case AudioDelay:
        return 1;
    default:
        return 0;
    }
}

unsigned char CMoviePlayer_Cocoa::CanGetProperty(MovieProperty property)
{
    switch (property) {
    case TouchSkippable:
    case AudioDelay:
        return 1;
    default:
        return 0;
    }
}
