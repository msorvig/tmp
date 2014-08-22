#ifndef _QT_RENDERER_
#define _QT_RENDERER_

#include <QWindow>

#include <QColor>
#include <QMutex>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QSharedPointer>
#include <QTimer>

#include "renderer.h"
#include "fbo.h"

class HelloWindow;
class QtRenderer : public QObject, public Renderer
{
    Q_OBJECT

public:
    explicit QtRenderer(const QSurfaceFormat &format, QtRenderer *share = 0, QScreen *screen = 0);
    
    QSurfaceFormat format() const { return m_format; }

    void setAnimating(HelloWindow *window, bool animating);

private slots:
    void render();

private:
    void initialize();

    void createGeometry();
    void createBubbles(int number);
    void quad(qreal x1, qreal y1, qreal x2, qreal y2, qreal x3, qreal y3, qreal x4, qreal y4);
    void extrude(qreal x1, qreal y1, qreal x2, qreal y2);

    qreal m_fAngle;

    QVector<QVector3D> vertices;
    QVector<QVector3D> normals;
    int vertexAttr;
    int normalAttr;
    int matrixUniform;
    int colorUniform;

    bool m_initialized;
    QSurfaceFormat m_format;
    QOpenGLContext *m_context;
    QOpenGLShaderProgram *m_program;
    QOpenGLBuffer m_vbo;
    FrameBufferObjectController *m_fboController;

    QList<HelloWindow *> m_windows;
    int m_currentWindow;

    QMutex m_windowLock;
    
};

#endif

