#ifndef _QUAD_RENDERER_
#define _QUAD_RENDERER_

#include <QWindow>

#include <QColor>
#include <QMutex>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QSharedPointer>
#include <QMatrix4x4>
#include <QQuaternion>
#include <QVector2D>
#include <QOpenGLTexture>
#include <QTimer>

#include "renderer.h"
#include "geometryengine.h"
#include "fbo.h"

class HelloWindow;
class QuadRenderer : public QObject, public Renderer, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit QuadRenderer(const QSurfaceFormat &format, QuadRenderer *share = 0, QScreen *screen = 0);
    
    QSurfaceFormat format() const { return m_format; }

    void setAnimating(HelloWindow *window, bool animating);

private slots:
    void render();

private:
    void initialize();


    bool m_initialized;
    QSurfaceFormat m_format;
    QOpenGLContext *m_context;
    FrameBufferObjectController *m_fboController;

    QOpenGLShaderProgram program;
    GeometryEngine *geometries;
    QOpenGLTexture *texture;
    QMatrix4x4 projection;

    QVector2D mousePressPosition;
    QVector3D rotationAxis;
    qreal angularSpeed;
    QQuaternion rotation;

    QList<HelloWindow *> m_windows;
    int m_currentWindow;
    QMutex m_windowLock;
};

#endif

