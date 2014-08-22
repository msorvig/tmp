#include "qtrenderer.h"
#include "hellowindow.h"
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <qmath.h>

QtRenderer::QtRenderer(const QSurfaceFormat &format, QtRenderer *share, QScreen *screen)
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

void QtRenderer::setAnimating(HelloWindow *window, bool animating)
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

void QtRenderer::render()
{
    QMutexLocker locker(&m_windowLock);

    if (m_windows.isEmpty())
        return;

    HelloWindow *surface = m_windows.at(m_currentWindow);
    QColor color = surface->color();

    m_currentWindow = (m_currentWindow + 1) % m_windows.size();

    if (!m_context->makeCurrent(surface))
        return;
    
    m_fboController->bind(surface->size());
    m_fboController->bindAndDraw();
    m_fboController->release();

    QSize viewSize = surface->size();

    locker.unlock();

    if (!m_initialized) {
        initialize();
        m_initialized = true;
    }


    //f->glClearColor(0.1f, 0.1f, 0.2f, 1.0f);

    QOpenGLFunctions *f = m_context->functions();
    f->glViewport(0, 0, viewSize.width() * surface->devicePixelRatio(), viewSize.height() * surface->devicePixelRatio());
    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    QMatrix4x4 modelview;
    modelview.rotate(m_fAngle, 0.0f, 1.0f, 0.0f);
    modelview.rotate(m_fAngle, 1.0f, 0.0f, 0.0f);
    modelview.rotate(m_fAngle, 0.0f, 0.0f, 1.0f);
    modelview.translate(0.0f, -0.2f, 0.0f);

    m_program->setUniformValue(matrixUniform, modelview);
    m_program->setUniformValue(colorUniform, color);

    m_context->functions()->glDrawArrays(GL_TRIANGLES, 0, vertices.size());

    m_context->swapBuffers(surface);

    m_fAngle += 1.0f;

    QTimer::singleShot(250, this, SLOT(render()));
}

void QtRenderer::initialize()
{
    QOpenGLShader *vshader = new QOpenGLShader(QOpenGLShader::Vertex, this);
    vshader->compileSourceCode(
        "attribute highp vec4 vertex;"
        "attribute mediump vec3 normal;"
        "uniform mediump mat4 matrix;"
        "uniform lowp vec4 sourceColor;"
        "varying mediump vec4 color;"
        "void main(void)"
        "{"
        "    vec3 toLight = normalize(vec3(0.0, 0.3, 1.0));"
        "    float angle = max(dot(normal, toLight), 0.0);"
        "    vec3 col = sourceColor.rgb;"
        "    color = vec4(col * 0.2 + col * 0.8 * angle, 1.0);"
        "    color = clamp(color, 0.0, 1.0);"
        "    gl_Position = matrix * vertex;"
        "}");

    QOpenGLShader *fshader = new QOpenGLShader(QOpenGLShader::Fragment, this);
    fshader->compileSourceCode(
        "varying mediump vec4 color;"
        "void main(void)"
        "{"
        "    gl_FragColor = color;"
        "}");

    m_program = new QOpenGLShaderProgram(this);
    m_program->addShader(vshader);
    m_program->addShader(fshader);
    m_program->link();
    m_program->bind();

    vertexAttr = m_program->attributeLocation("vertex");
    normalAttr = m_program->attributeLocation("normal");
    matrixUniform = m_program->uniformLocation("matrix");
    colorUniform = m_program->uniformLocation("sourceColor");

    m_fAngle = 0;
    createGeometry();

    m_vbo.create();
    m_vbo.bind();
    const int verticesSize = vertices.count() * 3 * sizeof(GLfloat);
    m_vbo.allocate(verticesSize * 2);
    m_vbo.write(0, vertices.constData(), verticesSize);
    m_vbo.write(verticesSize, normals.constData(), verticesSize);

    QOpenGLFunctions *f = m_context->functions();
    f->glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
    f->glFrontFace(GL_CW);
    f->glCullFace(GL_FRONT);
    f->glEnable(GL_CULL_FACE);
    f->glEnable(GL_DEPTH_TEST);

    m_program->enableAttributeArray(vertexAttr);
    m_program->enableAttributeArray(normalAttr);
    m_program->setAttributeBuffer(vertexAttr, GL_FLOAT, 0, 3);
    m_program->setAttributeBuffer(normalAttr, GL_FLOAT, verticesSize, 3);
}

void QtRenderer::createGeometry()
{
    vertices.clear();
    normals.clear();

    qreal x1 = +0.06f;
    qreal y1 = -0.14f;
    qreal x2 = +0.14f;
    qreal y2 = -0.06f;
    qreal x3 = +0.08f;
    qreal y3 = +0.00f;
    qreal x4 = +0.30f;
    qreal y4 = +0.22f;

    quad(x1, y1, x2, y2, y2, x2, y1, x1);
    quad(x3, y3, x4, y4, y4, x4, y3, x3);

    extrude(x1, y1, x2, y2);
    extrude(x2, y2, y2, x2);
    extrude(y2, x2, y1, x1);
    extrude(y1, x1, x1, y1);
    extrude(x3, y3, x4, y4);
    extrude(x4, y4, y4, x4);
    extrude(y4, x4, y3, x3);

    const qreal Pi = 3.14159f;
    const int NumSectors = 100;

    for (int i = 0; i < NumSectors; ++i) {
        qreal angle1 = (i * 2 * Pi) / NumSectors;
        qreal x5 = 0.30 * qSin(angle1);
        qreal y5 = 0.30 * qCos(angle1);
        qreal x6 = 0.20 * qSin(angle1);
        qreal y6 = 0.20 * qCos(angle1);

        qreal angle2 = ((i + 1) * 2 * Pi) / NumSectors;
        qreal x7 = 0.20 * qSin(angle2);
        qreal y7 = 0.20 * qCos(angle2);
        qreal x8 = 0.30 * qSin(angle2);
        qreal y8 = 0.30 * qCos(angle2);

        quad(x5, y5, x6, y6, x7, y7, x8, y8);

        extrude(x6, y6, x7, y7);
        extrude(x8, y8, x5, y5);
    }

    for (int i = 0;i < vertices.size();i++)
        vertices[i] *= 2.0f;
}

void QtRenderer::quad(qreal x1, qreal y1, qreal x2, qreal y2, qreal x3, qreal y3, qreal x4, qreal y4)
{
    vertices << QVector3D(x1, y1, -0.05f);
    vertices << QVector3D(x2, y2, -0.05f);
    vertices << QVector3D(x4, y4, -0.05f);

    vertices << QVector3D(x3, y3, -0.05f);
    vertices << QVector3D(x4, y4, -0.05f);
    vertices << QVector3D(x2, y2, -0.05f);

    QVector3D n = QVector3D::normal
        (QVector3D(x2 - x1, y2 - y1, 0.0f), QVector3D(x4 - x1, y4 - y1, 0.0f));

    normals << n;
    normals << n;
    normals << n;

    normals << n;
    normals << n;
    normals << n;

    vertices << QVector3D(x4, y4, 0.05f);
    vertices << QVector3D(x2, y2, 0.05f);
    vertices << QVector3D(x1, y1, 0.05f);

    vertices << QVector3D(x2, y2, 0.05f);
    vertices << QVector3D(x4, y4, 0.05f);
    vertices << QVector3D(x3, y3, 0.05f);

    n = QVector3D::normal
        (QVector3D(x2 - x4, y2 - y4, 0.0f), QVector3D(x1 - x4, y1 - y4, 0.0f));

    normals << n;
    normals << n;
    normals << n;

    normals << n;
    normals << n;
    normals << n;
}

void QtRenderer::extrude(qreal x1, qreal y1, qreal x2, qreal y2)
{
    vertices << QVector3D(x1, y1, +0.05f);
    vertices << QVector3D(x2, y2, +0.05f);
    vertices << QVector3D(x1, y1, -0.05f);

    vertices << QVector3D(x2, y2, -0.05f);
    vertices << QVector3D(x1, y1, -0.05f);
    vertices << QVector3D(x2, y2, +0.05f);

    QVector3D n = QVector3D::normal
        (QVector3D(x2 - x1, y2 - y1, 0.0f), QVector3D(0.0f, 0.0f, -0.1f));

    normals << n;
    normals << n;
    normals << n;

    normals << n;
    normals << n;
    normals << n;
}
