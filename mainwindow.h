#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QLabel>
#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSettings>
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
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void updateAccessTokenStatus();
    void checkDataspaceAvailability();
    void dataspaceReplyFinished(QNetworkReply *reply);
    void loadAccessToken();
    void configureProxy();
    void promptUserCredentialsAndRequestToken();

    Ui::MainWindow *ui;
    QLabel *accessTokenStatusLabel;
    QTimer *checkTimer;
    QNetworkAccessManager *networkManager;
};
#endif // MAINWINDOW_H
