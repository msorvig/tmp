#include <QtGui>
#include <QtWidgets>

#include <Cocoa/Cocoa.h>

// This exammple show how to drive animations on several
// NSViews (raster and OpenGL), on the GUi thread, while
// at the same time time responding to user input such at
// window resize and mouse events.
//
// The apraoch used is to call setNeedsDisplay on timer
// and other events, and then drive all painting from drawRecy
//
// Useful Apple sample code:
//   OSXGLEssentials
//   LayerBackedOpenGLView
// Stack Owerflow:
//   http://stackoverflow.com/questions/7610117/layer-backed-openglview-redraws-only-if-window-is-resized
//
// Note: The animations are advaned per-frame, and will slow
// down on skipped frames.
// Note 2: Updating a view will sometimes trigger a redraw on
// ajacent OpenGL views
//


// #define SINGLE_VIEW 1
#define ANIMATE 1
//#define LAYER_BACKED 1

// Misc:
// #define DISPLAY_LINK 1
// #define RECURRING_TMER 1
//#define OPENGL_DISPLAYLINK 1
#define OPENGL_DOUBLE_BUFFERED 1

int viewSize = 300;
float timerInterval = 0.01;

CVReturn animatedViewDisplayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp* now, const CVTimeStamp* outputTime, 
CVOptionFlags flagsIn, CVOptionFlags* flagsOut, void* displayLinkContext);


// raster-based view
@interface AnimatedView : NSView {
    NSTimer *m_timer;
    int m_frameCounter;
    NSPoint m_pos;
    CVDisplayLinkRef m_displayLink;
}
@end

@implementation AnimatedView

- (id) init
{
    [super init];
    m_frameCounter = 0;
    
#if defined(LAYER_BACKED)
    [self setWantsLayer:YES];
#endif    

    CVDisplayLinkCreateWithActiveCGDisplays(&m_displayLink);
    CVDisplayLinkSetOutputCallback(m_displayLink, &animatedViewDisplayLinkCallback, self);
    CVDisplayLinkSetCurrentCGDisplay(m_displayLink, kCGDirectMainDisplay);

#if defined(ANIMATE) && defined (RECURRING_TMER)
    m_timer = [NSTimer timerWithTimeInterval:timerInterval
                            target:self
                          selector:@selector(requestUpdateTimerFire)
                          userInfo:nil
                           repeats:YES];

    NSRunLoop *runloop = [NSRunLoop currentRunLoop];
    [runloop addTimer:m_timer forMode:NSRunLoopCommonModes];
    [runloop addTimer:m_timer forMode:NSEventTrackingRunLoopMode];
#endif

    return self;
}

- (void)drawRect:(NSRect)dirtyRect
{
    //qDebug("drawRect");
    
    ++m_frameCounter;

    // blue backround 
    [[NSColor blueColor] setFill];
    NSRectFill(dirtyRect);

    // animated red square
    [[NSColor redColor] setFill];
    NSRectFill(NSMakeRect(10, m_frameCounter % viewSize, 100, 100));

    // dragable green square
    [[NSColor greenColor] setFill];
    NSRectFill(NSMakeRect(m_pos.x - 50, m_pos.y - 50, 100, 100));

    // requesting a new frame at draw time has no effect.
//    [self setNeedsDisplay:YES]; // nope

#if defined(ANIMATE) && defined (DISPLAY_LINK)
    if (!CVDisplayLinkIsRunning(m_displayLink))
           CVDisplayLinkStart(m_displayLink);
#else
#if defined(ANIMATE) && !defined (RECURRING_TMER)
    // request update
    m_timer = [NSTimer timerWithTimeInterval:timerInterval
                            target:self
                          selector:@selector(requestUpdateTimerFire)
                          userInfo:nil
                           repeats:NO];

    NSRunLoop *runloop = [NSRunLoop currentRunLoop];
    [runloop addTimer:m_timer forMode:NSRunLoopCommonModes];
    [runloop addTimer:m_timer forMode:NSEventTrackingRunLoopMode];
#endif
#endif
}

- (void) requestUpdateTimerFire
{
    [self setNeedsDisplay:YES];
}

- (void)mouseDown:(NSEvent *)theEvent
{
    m_pos = [self convertPoint:[theEvent locationInWindow] fromView:nil];
}

- (void)mouseUp:(NSEvent *)theEvent
{
    
}

- (void)mouseDragged:(NSEvent *)theEvent
{
    m_pos = [self convertPoint:[theEvent locationInWindow] fromView:nil];
    [self setNeedsDisplay:YES];
}

- (BOOL) isOpaque
{
    return YES;
}
 
@end

CVReturn animatedViewDisplayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp* now, const CVTimeStamp* outputTime,
                                        CVOptionFlags flagsIn, CVOptionFlags* flagsOut, void* displayLinkContext)
{
    AnimatedView *view = (AnimatedView*)displayLinkContext;
    [view requestUpdateTimerFire];
}

CVReturn openglViewDisplayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp* now, const CVTimeStamp* outputTime, 
CVOptionFlags flagsIn, CVOptionFlags* flagsOut, void* displayLinkContext)
{
    NSView *view = (AnimatedView*)displayLinkContext;
    [view drawContents];
}

 
@interface CustomOpenGLView : NSView
{
  @private
    NSOpenGLContext*     m_openGLContext;
    NSOpenGLPixelFormat* m_pixelFormat;
    NSTimer * m_timer;
    CVDisplayLinkRef m_displayLink;
        
    int m_frameCounter;
}
- (id)init;
- (void) surfaceNeedsUpdate:(NSNotification*)notification;
@end

@implementation CustomOpenGLView

- (id)init
{
    self = [super init];
    if (self != nil) {
        NSOpenGLPixelFormatAttribute colorSize = 32;
        NSOpenGLPixelFormatAttribute depthSize = 32;
        m_frameCounter = 0;
    
        NSOpenGLPixelFormatAttribute windowedAttributes[] =
        {
//            NSOpenGLPFANoRecovery,
            NSOpenGLPFAColorSize, colorSize,
            NSOpenGLPFADepthSize, depthSize,
    #if OPENGL_DOUBLE_BUFFERED
            NSOpenGLPFADoubleBuffer,
    #endif
            NSOpenGLPFAAccelerated,
            0
        };
        
#if defined(LAYER_BACKED)
    [self setWantsLayer:YES];
#endif    

        m_pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes: windowedAttributes];
       // [m_pixelFormat autorelease];
        
        m_openGLContext = [[NSOpenGLContext alloc] initWithFormat:m_pixelFormat shareContext:nil];
    //    [m_openGLContext autorelease];
        
#if defined (ANIMATE) && defined(OPENGL_DISPLAYLINK)
    	CVDisplayLinkCreateWithActiveCGDisplays(&m_displayLink);
    	CVDisplayLinkSetOutputCallback(m_displayLink, &openglViewDisplayLinkCallback, self);
    	CGLContextObj cglContext = [m_openGLContext CGLContextObj];
    	CGLPixelFormatObj cglPixelFormat = [cglContext CGLPixelFormatObj];
        CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(m_displayLink, cglContext, cglPixelFormat);
    	CVDisplayLinkStart(m_displayLink);
#endif        
        
    [[NSNotificationCenter defaultCenter] addObserver:self
                     selector:@selector(surfaceNeedsUpdate:)
                     name:NSViewGlobalFrameDidChangeNotification
                     object:self];
    }
    return self;
}

- (void) surfaceNeedsUpdate:(NSNotification*)notification
{
    [m_openGLContext update];
}

- (BOOL)isOpaque
{
    return YES;
}

- (void) drawContents
{
    ++m_frameCounter;
    
 //   CGLLockContext([m_openGLContext CGLContextObj]);
    
    [m_openGLContext makeCurrentContext];

    NSRect bounds = [self bounds];
    glViewport(bounds.origin.x, bounds.origin.y, bounds.size.width,
               bounds.size.height);

    // animate clear color
    float color = (sin(m_frameCounter / 30.0f) + 2.0f )/ 4.0;
    glClearColor(color, color, color, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glFinish();
    [m_openGLContext flushBuffer];
    
 //   CGLUnlockContext([m_openGLContext CGLContextObj]);
}

-(void) drawRect:(NSRect)dirtyRect
{
    if ([m_openGLContext view] != self)
        [m_openGLContext setView:self];

    [self drawContents];

#if defined(ANIMATE) && !defined (RECURRING_TMER) && !defined (OPENGL_DISPLAYLINK)
    // request update
    m_timer = [NSTimer timerWithTimeInterval:timerInterval
                            target:self
                          selector:@selector(requestUpdateTimerFire)
                          userInfo:nil
                           repeats:NO];

    NSRunLoop *runloop = [NSRunLoop currentRunLoop];
    [runloop addTimer:m_timer forMode:NSRunLoopCommonModes];
    [runloop addTimer:m_timer forMode:NSEventTrackingRunLoopMode];
#endif
}

- (void) requestUpdateTimerFire
{
    [self setNeedsDisplay:YES];
}

-(void) viewDidMoveToWindow
{
    [super viewDidMoveToWindow];
    if ([self window] == nil)
        [m_openGLContext clearDrawable];
}

@end // CustomOpenGLView

@interface OpaqueView : NSView
@end

@implementation OpaqueView
-(BOOL) isOpaque
{
    return YES;
}
@end

class OSXCustomWindow : public QObject
{
public:
    OSXCustomWindow()
    {
        // Create OSXCustomWindow
        NSRect frame = NSMakeRect(500, 500, viewSize * 2, viewSize * 2);
        m_window  = [[NSWindow alloc] 
            initWithContentRect:frame
                      styleMask:NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask | NSResizableWindowMask
                        backing:NSBackingStoreBuffered
                          defer:NO];

    }
    
    void createView()
    {
#ifdef SINGLE_VIEW
//        [m_window setContentView: [[AnimatedView alloc] init]];
        [m_window setContentView: [[CustomOpenGLView alloc] init]];
#else
        [m_window setContentView: [[OpaqueView alloc] init]];
        NSView *contentView = [m_window contentView];
        contentView.autoresizesSubviews = YES;
        
        NSView *animatedView1 = [[AnimatedView alloc] init];
        animatedView1.frame = NSMakeRect(0,0, viewSize, viewSize);
        animatedView1.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable |  NSViewMinXMargin | NSViewMaxXMargin  |  NSViewMinYMargin  | NSViewMaxYMargin ;
        [contentView addSubview: animatedView1];
        
        NSView *animatedView2 = [[AnimatedView alloc] init];
        animatedView2.frame = NSMakeRect(viewSize, viewSize, viewSize, viewSize);
        animatedView2.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable |  NSViewMinXMargin | NSViewMaxXMargin  |  NSViewMinYMargin  | NSViewMaxYMargin ;
        [contentView addSubview: animatedView2];

        NSView *openglView1 = [[CustomOpenGLView alloc] init];
        openglView1.frame = NSMakeRect(0, viewSize, viewSize, viewSize);
        openglView1.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable |  NSViewMinXMargin | NSViewMaxXMargin  |  NSViewMinYMargin  | NSViewMaxYMargin ;
        [contentView addSubview: openglView1];

        NSView *openglView2 = [[CustomOpenGLView alloc] init];
        openglView2.frame = NSMakeRect(viewSize, 0, viewSize, viewSize);
        openglView2.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable |  NSViewMinXMargin | NSViewMaxXMargin  |  NSViewMinYMargin  | NSViewMaxYMargin ;
        [contentView addSubview: openglView2];
#endif                
    }
    
    void show()
    {
        [m_window makeKeyAndOrderFront:NSApp];
    }
    
    NSWindow *m_window;
};


#ifdef QT_APPLICATION
// WindowController is a simple target functor for
// QTimer::singleShot() below.
class WindowController
{
public:
    
    void operator()()
    {
        m_window.reset(new OSXCustomWindow());
        m_window->createView();
        m_window->show();
    }
private:
    QSharedPointer<OSXCustomWindow> m_window;
};

#else

@interface WindowController : NSObject {}
- (void)createWindow;
@end

@implementation WindowController
- (void)createWindow {
    OSXCustomWindow *m_window = new OSXCustomWindow();
    m_window->createView();
    m_window->show();
}
@end
#endif

int main(int argc, char **argv)
{
#ifdef QT_APPLICATION
    QApplication app(argc, argv);
    
    // Create and show the window after the event loop 
    // has been started with exec() below.
    WindowController windowController;
    QTimer::singleShot(10, windowController);

    return app.exec();
#else
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
    [NSApplication sharedApplication];

    // schedule call to create the UI.
    WindowController *windowController = [WindowController alloc];
    [NSTimer scheduledTimerWithTimeInterval:timerInterval
             target:windowController
             selector:@selector(createWindow)
             userInfo:nil 
             repeats:NO];

    [(NSApplication *)NSApp run];
    [NSApp release];
    [pool release];
    return 0;
#endif
}
