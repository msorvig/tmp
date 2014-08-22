#include <fbo.h>

#include <QScopedPointer>
#include <qdebug.h>

FrameBufferObjectController::FrameBufferObjectController()
    :initialized(false), m_fbo(0), m_context(0), m_shareContext(0), m_surface(0)
{

}

void FrameBufferObjectController::setShareContext(QOpenGLContext *shareContext)
{
    m_shareContext = shareContext;
}

void FrameBufferObjectController::setSize(QSize size)
{
    m_size = size;
}

bool FrameBufferObjectController::bind(const QSize &size)
{
//    qDebug() << "bind" << currentFramebufferBidning();

    if (!m_fbo || m_fbo->size() != size) {
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);

        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &m_systemFbo);
//        qDebug() << "system fbo" << m_systemFbo;

        QScopedPointer<QOpenGLFramebufferObject> newFBO(new QOpenGLFramebufferObject(size, format));
//        qDebug() << "newFBO" << newFBO->isValid();
        
        bool ok = newFBO->bind();
//        qDebug() << "new-bind" << ok;

        if (ok) {
            delete m_fbo;
            m_fbo = newFBO.take();
            return true;
        }
    } else {
        bool ok = m_fbo->bind();
//        qDebug() << "re-bind" << ok;
        return ok;
    }
    return true;
}

void FrameBufferObjectController::release()
{
//    qDebug() << "release 1" << m_fbo->isBound();
    m_fbo->release();
//    qDebug() << "after release" << currentFramebufferBidning();
}

void FrameBufferObjectController::bindAndDraw()
{
//    qDebug() << "bindAndDraw" << m_context << QOpenGLContext::currentContext();

    if (!initialized) {
        initialize();
        initialized = true;
    } else {
        if (m_context) {
            if(!m_context->makeCurrent(m_surface))
                qFatal("Could not make FBO context current");
        }
    }
    
//    qDebug() << "bind" << m_context << QOpenGLContext::currentContext() << m_context->isValid();
    
    bind(m_size);
    
    glPushAttrib(GL_CURRENT_BIT);
    glPushAttrib(GL_COLOR_BUFFER_BIT);

    glClearColor(0.7, 0.2, 0.5, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glViewport(0 ,0, m_size.width(), m_size.height());

    static double degree = 0;
    degree += 0.01;
    GLfloat rotate = degree * 60.0;

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glRotatef(rotate, 0.0, 0.0, 1.0);
    glBegin(GL_QUADS);
            glColor3f(1.0, 0.0, 0.0);
            glVertex2f(-0.5, -0.5);
            glVertex2f(-0.5,  0.5);
            glVertex2f( 0.5,  0.5);
            glVertex2f( 0.5, -0.5);
    glEnd();
    glPopMatrix();
    
    glPopAttrib();
    
    if (m_context) {
        glFlush();
        m_context->doneCurrent();
    }
}


void FrameBufferObjectController::initialize()
{
    if (!m_shareContext)
        return;
    
    qDebug() << "FrameBufferObjectController::initialize";

    QScopedPointer<QOpenGLContext> ctx(new QOpenGLContext);
    ctx->setShareContext(m_shareContext);
    ctx->setFormat(m_shareContext->format());

    if (!ctx->create()) {
        qWarning("QOpenGLWidget: Failed to create context");
        return;
    }

    QOffscreenSurface *surface = new QOffscreenSurface;
    surface->setFormat(ctx->format());
    surface->create();
    m_surface = surface;

    if (!ctx->makeCurrent(surface)) {
        qWarning("QOpenGLWidget: Failed to make context current");
        return;
    }
    
    qDebug() << "FrameBufferObjectController::initialize" << QOpenGLContext::currentContext();
    
    m_context = ctx.take();
}

GLint FrameBufferObjectController::texture()
{
    return m_fbo->texture();    
}

void FrameBufferObjectController::blit()
{
    if (m_fbo && m_fbo->isValid()) {
        GLint defaultReadFBO = 0;
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &defaultReadFBO);
        
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo->handle());
        
        // TODO: The sizes should be the same actually, fix it.
        const QSize fboSize(m_fbo->size());
        glBlitFramebuffer(0, 0, fboSize.width(), fboSize.height(),
                          0, 0, fboSize.width(), fboSize.height(),
                          GL_COLOR_BUFFER_BIT, GL_LINEAR);

        // TODO: check if I can somehow restore the previous one.
        glBindFramebuffer(GL_READ_FRAMEBUFFER, defaultReadFBO);
    }
}

int FrameBufferObjectController::currentFramebufferBidning()
{
    GLint binding;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &binding);
    return binding;
}
