#include "quadrenderer.h"
#include "hellowindow.h"
#include "geometryengine.h"
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <qmath.h>

QuadRenderer::QuadRenderer(const QSurfaceFormat &format, QuadRenderer *share, QScreen *screen)
    : m_initialized(false)
    , m_format(format)
    , m_currentWindow(0)
{
    m_context = new QOpenGLContext(this);
    if (screen)
        m_context->setScreen(screen);
    m_context->setFormat(format);
    if (share)
        m_context->setShareContext(share->m_context);
    m_context->create();
    
    m_fboController = new FrameBufferObjectController();
}

void QuadRenderer::setAnimating(HelloWindow *window, bool animating)
{
    QMutexLocker locker(&m_windowLock);
    if (m_windows.contains(window) == animating)
        return;

    if (animating) {
        m_windows << window;
        if (m_windows.size() == 1)
            QTimer::singleShot(0, this, SLOT(render()));
    } else {
        m_currentWindow = 0;
        m_windows.removeOne(window);
    }
}

void QuadRenderer::render()
{
//    qDebug() << "render";

    HelloWindow *surface = m_windows.at(m_currentWindow);
    QColor color = surface->color();

    m_currentWindow = (m_currentWindow + 1) % m_windows.size();
    QSize viewSize = surface->size();
    QSize contextSize = surface->size() * surface->devicePixelRatio();

    bool useShareContext = true;
    if (useShareContext) {
        m_fboController->setShareContext(m_context);
        m_fboController->setSize(contextSize);
        m_fboController->bindAndDraw();
        m_fboController->release();

        if (!m_context->makeCurrent(surface))
            return;

    } else {
        if (!m_context->makeCurrent(surface))
            return;

        m_fboController->setSize(surface->size());
        m_fboController->bindAndDraw();
        m_fboController->release();
    }

    if (!m_initialized) {
        initialize();
        m_initialized = true;
    }

//    m_context->makeCurrent(surface);
#if 0
    // Perspective projection matrix
    qreal aspect = qreal(viewSize.width()) / qreal(viewSize.height() ? viewSize.height() : 1);
    const qreal zNear = 3.0, zFar = 7.0, fov = 45.0;
    projection.setToIdentity();
    projection.perspective(fov, aspect, zNear, zFar);

    // Calculate model view transformation
    QMatrix4x4 matrix;
    matrix.translate(0.0, 0.0, -5.0);
    matrix.rotate(rotation);

    program.setUniformValue("mvp_matrix", projection * matrix);
#else
    // Orthographic projection matrix
    QMatrix4x4 matrix;
    matrix.setToIdentity();
    matrix.ortho(-1, 1 , -1, 1, -100, 100);
    program.setUniformValue("mvp_matrix", matrix);
#endif    
    
    // Clear color and depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Texturing:
    bool useFboTexture = true;
    if (useFboTexture) {
        GLint fboTexture = m_fboController->texture();
        glBindTexture(GL_TEXTURE_2D, fboTexture);
    } else {
        texture->bind();
    }
    program.setUniformValue("texture", 0);     // Use texture unit 0

    // draw
    bool blit = false;
    if (blit) {
        m_fboController->blit();
    } else {
        geometries->drawGeometry(&program);
    }
    m_context->swapBuffers(surface);

    QTimer::singleShot(0, this, SLOT(render()));
}

void QuadRenderer::initialize()
{
    qDebug() << "initialize";
    initializeOpenGLFunctions();

    glClearColor(0, 0, 0, 1);

    // shaders
    if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/vshader.glsl"))
        return;
    if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/fshader.glsl"))
        return;
    if (!program.link())
        return;;
    if (!program.bind())
        return;;

    // textures
    texture = new QOpenGLTexture(QImage(":/cube.png").mirrored());
    texture->setMinificationFilter(QOpenGLTexture::Nearest);
    texture->setMagnificationFilter(QOpenGLTexture::Linear);
    texture->setWrapMode(QOpenGLTexture::Repeat);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    geometries = new GeometryEngine;
    qDebug() << "initialize done";
}

