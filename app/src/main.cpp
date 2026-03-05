#include "mainwindow.h"
#include <QApplication>
#include <QTranslator>
#include <QLocale>

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
#include <QTextCodec>
#endif

int main(int argc, char *argv[])
{
    Q_INIT_RESOURCE(app);
    QApplication a(argc, argv);

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
#endif

    QTranslator translator;
    QString localeName = QLocale::system().name();
    if (translator.load(QString(":/app_%1.qm").arg(localeName))) {
        a.installTranslator(&translator);
    } else if (translator.load(QString("app_%1").arg(localeName), a.applicationDirPath())) {
        a.installTranslator(&translator);
    }

    MainWindow w;
    w.showMaximized();
    return a.exec();
}
