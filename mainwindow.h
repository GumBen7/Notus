#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QLabel>
#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    // QLabel *accessTokenStatusLabel;
    // QTimer *checkTimer;
    // void updateAccessTokenStatus();
    QNetworkAccessManager *networkManager;
    void checkDataspaceAvailability();
    void dataspaceReplyFinished(QNetworkReply *reply);
    void configureProxy();
};
#endif // MAINWINDOW_H
