#include <QtCore>
#include <QtGui>
#include <QtQuick>

QByteArray qmlview = "import QtQuick 2.1 \n"
                "\n"
                " Text {\n"
                " text: \"hello world\"\n"
                "x: 20\n"
                "y: 20\n"
        "width: 300\n"
        "height: 300\n"
"}\n";

class QmlReloader : public QObject
{
    Q_OBJECT
public:
    enum Status {
        LoadOk,
        LoadError
    };
 
    QmlReloader()
    : baseUrl("/")
    , view(0)
    , component(0)
    {
        view = new QQuickView();
        view->show();
        
        view->setResizeMode(QQuickView::SizeRootObjectToView);
    }
    
    void loadSource(const QByteArray &source)
    {
        delete component;
        view->engine()->clearComponentCache();
        component = new QQmlComponent(view->engine());
        QUrl baseUrl("/");
        component->setData(source, baseUrl);
        if (component->isLoading()) {
             QObject::connect(component, SIGNAL(statusChanged(QQmlComponent::Status)),
                              this, SLOT(continueLoading()));
         } else {
             continueLoading();
         }
    }
    
    QList<QQmlError> errors()
    {
        if (!component)
            return QList<QQmlError>();
        return component->errors();
    }
    
private Q_SLOTS:
    void continueLoading()
    {
        if (component->isError()) {
            emit statusChanged(LoadError);
        } else {
            QQuickItem *item = qobject_cast<QQuickItem *>(component->create());
            view->setContent(QUrl(), component, item);
            emit statusChanged(LoadOk);
        }
    }
Q_SIGNALS:
    void statusChanged(Status status);
    
private:
    QUrl baseUrl;
    QQuickView *view;
    QQmlComponent *component;
};

QByteArray loadFile(const QString &fileName)
{
    QFile file(fileName);
    file.open(QIODevice::ReadOnly);
    return file.readAll();
}

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    QString fileName = "main.qml";

    // Create QML realoader
    QmlReloader reloader;
    auto reload = [&reloader] (const QString &fileName) { reloader.loadSource(loadFile(fileName)); };

    // Error handling
    auto printStatus = [&reloader](QmlReloader::Status status){
        qDebug() << "status is" << status;
        if (status == QmlReloader::LoadError) {
            qDebug() << "error" << reloader.errors();
        }
    };
    QObject::connect(&reloader, &QmlReloader::statusChanged, printStatus);

    // Filesystem watcher
    QFileSystemWatcher watcher;
    watcher.addPath(fileName);
    QObject::connect(&watcher, &QFileSystemWatcher::fileChanged, reload);

    // Initial load
    reload(fileName);

    return app.exec();
}

#include "main.moc"
