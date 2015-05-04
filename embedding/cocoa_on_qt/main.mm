#include <QtQuick>
#include <QtGui>
#import <Cocoa/Cocoa.h>

int main(int argc, char **argv)
{
    QGuiApplication a(argc, argv);

    // Layer OpenGL below NSWindow/NSView content
    qputenv("QT_MAC_OPENGL_SURFACE_ORDER", "-1");

    // Alternativly, use a CA layer (which currenly
    // requires using the basic render looop)
    // qputenv("QT_MAC_WANTS_LAYER", "1");
    // qputenv("QSG_RENDER_LOOP", "basic");

    QQuickView view;
    view.setSource(QUrl::fromLocalFile("main.qml"));
    view.resize(512, 512);

    // Create the native NSView, use QWindow::fromWinId to warp it in a QWindow
    NSTextView *textField1 = [[[NSTextView alloc] init] autorelease];
    [textField1 insertText: @"NSTexView 1"];
    QScopedPointer<QWindow> window1(QWindow::fromWinId(WId(textField1)));
    window1->setGeometry(10,10,200,100);
    window1->setParent(&view);

    // Another one
    NSTextView *textField2 = [[[NSTextView alloc] init] autorelease];
    [textField2 insertText: @"NSTexView 2"];
    QScopedPointer<QWindow> window2(QWindow::fromWinId(WId(textField2)));
    window2->setGeometry(250,10,200,100);
    window2->setParent(&view);

    // Show the top-level QQuickView window (no need to show the NSTextView window)
    view.show();
    return a.exec();
}
