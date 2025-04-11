#include "mainwindow.h"
#include "qt6keychain/keychain.h"
#include "ui_mainwindow.h"

#include <QNetworkProxy>

constexpr int kHttpOk = 200;
constexpr auto kUserAgentHeader = "Qt App";
constexpr auto kBlockedResponseSubstring = "The requested URL was rejected";
inline const QString kHttpsScheme = "https://";
inline const QString kDataspaceHostname = "dataspace.copernicus.eu";

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , networkManager(new QNetworkAccessManager(this))
{
    ui->setupUi(this);
    configureProxy();
    checkDataspaceAvailability();
    connect(networkManager, &QNetworkAccessManager::finished, this, &MainWindow::dataspaceReplyFinished);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::checkDataspaceAvailability()
{
    QUrl url(kHttpsScheme + kDataspaceHostname);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, kUserAgentHeader);
    networkManager->get(request);
}

void MainWindow::dataspaceReplyFinished(QNetworkReply *reply)
{
    int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    auto responseBody = reply->readAll();
    auto palette = ui->dataspaceStatusLabel->palette();
    if (reply->error() == QNetworkReply::NoError) {
        if (httpStatus == kHttpOk && responseBody.contains(kBlockedResponseSubstring)) {
            ui->dataspaceStatusLabel->setText(kDataspaceHostname + ": Доступ ограничен");
            palette.setColor(QPalette::WindowText, Qt::yellow);
        } else {
            ui->dataspaceStatusLabel->setText(kDataspaceHostname + ": Доступен");
            palette.setColor(QPalette::WindowText, Qt::green);
            loadAccessToken();
        }
    } else {
        ui->dataspaceStatusLabel->setText(kDataspaceHostname + ": Недоступен ("
                                          + reply->errorString() + ")");
        palette.setColor(QPalette::WindowText, Qt::red);
    }
    ui->dataspaceStatusLabel->setStyleSheet("");
    ui->dataspaceStatusLabel->setPalette(palette);
    reply->deleteLater();
}

void MainWindow::loadAccessToken()
{
    /* TODO(GumBen7): implement loading from keychain  using namespace QKeychain;
    auto job = new QKeychain::ReadPasswordJob(QCoreApplication::applicationName(), this);
    job->setKey("access_token");
    connect(job, &QKeychain::Job::finished, this, [=]() {
        if (job->error()) {
            qDebug() << "Token not found in keychain, requesting login...";
        } else {
            QString token = job->textData();
            qDebug() << "Access token loaded from keychain:" << token;
        }
    });
    job->start();*/
}

void MainWindow::configureProxy()
{
    QSettings settings;
    QNetworkProxy proxy;
    proxy.setType(QNetworkProxy::HttpProxy);
    proxy.setHostName(settings.value("Proxy/host").toString());
    proxy.setPort(settings.value("Proxy/port").toInt());
    QNetworkProxy::setApplicationProxy(proxy);
}
