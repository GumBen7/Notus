#include "mainwindow.h"

#include <QApplication>
#include <QDebug>
#include <QLocale>
#include <QTranslator>

constexpr int kProxyPortDefault = 3128;
constexpr auto kProxyHostDefault = "54.37.207.54";

static void InitializeSettingsIfNeeded()
{
    QSettings settings("GumBen7", "Notus");
    if (!settings.contains("Proxy/host") || !settings.contains("Proxy/port")) {
        settings.beginGroup("Proxy");
        settings.setValue("host", kProxyHostDefault);
        settings.setValue("port", kProxyPortDefault);
        settings.endGroup();
        qDebug() << "Proxy settings initialized: host =" << kProxyHostDefault
                 << ", port =" << kProxyPortDefault;
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("GumBen7");
    QCoreApplication::setApplicationName("Notus");
    try {
        QApplication app(argc, argv);
        InitializeSettingsIfNeeded();
        QTranslator translator;
        const QStringList uiLanguages = QLocale::system().uiLanguages();
        for (const QString &locale : uiLanguages) {
            const QString baseName = "Notus_" + QLocale(locale).name();
            if (translator.load(":/i18n/" + baseName)) {
                app.installTranslator(&translator);
                break;
            }
        }
        MainWindow w;
        w.show();
        return app.exec();
    } catch (const std::exception &e) {
        qCritical() << "Fatal error:" << e.what();
        return EXIT_FAILURE;
    }
}
