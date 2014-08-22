#ifndef _FBO_H_
#define _FBO_H_

#include <QOpenGLFunctions>
#include <QOpenGLFramebufferObject>
#include <QOpenGLContext>
#include <QOffscreenSurface>

class FrameBufferObjectController
{
public:
    FrameBufferObjectController();

    void setShareContext(QOpenGLContext *shareContext = 0);
    void setSize(QSize size);
    bool bind(const QSize &size);
    void release();
    void bindAndDraw();

    GLint texture();
    void blit();
    int currentFramebufferBidning();
    void initialize();
private:
    bool initialized;
    QSize m_size;
    QOpenGLFramebufferObject *m_fbo;
    GLint m_systemFbo;

    QOpenGLContext *m_context;
    QOpenGLContext *m_shareContext;
    QOffscreenSurface *m_surface;
};

#endif
