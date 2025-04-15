#include "mainwindow.h"
#include "qt6keychain/keychain.h"
#include "ui_mainwindow.h"

#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkProxy>
#include <QUrlQuery>

#include <QProcess>

constexpr int kHttpOk = 200;
constexpr auto kUserAgentHeader = "Notus/1.0 (Qt 6.x)";
constexpr auto kBlockedResponseSubstring = "The requested URL was rejected";
constexpr auto kTokenKeyName = "CopernicusAccessToken";
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
    auto job = new QKeychain::ReadPasswordJob(QCoreApplication::applicationName(), this);
    job->setKey(kTokenKeyName);
    connect(job, &QKeychain::Job::finished, this, [=]() {
        if (job->error()) {
            qDebug() << "Token not found in keychain, requesting login...";
            promptUserCredentialsAndRequestToken();
        } else {
            QString token = job->textData();
            qDebug() << "Access token loaded from keychain:" << token;
        }
    });
    job->start();
}

void MainWindow::promptUserCredentialsAndRequestToken()
{
    QProcess curl;
    curl.start("curl",
               QStringList() << "-s" << "-X" << "POST"
                             << "https://identity.dataspace.copernicus.eu/auth/realms/CDSE/"
                                "protocol/openid-connect/token"
                             << "-d" << "username=gumben7@gmail.com" << "-d"
                             << "password=vhR5DT!rpw8Bb.f"
                             << "-d" << "grant_type=password" << "-d" << "client_id=cdse-public");
    curl.waitForFinished();
    QByteArray output = curl.readAllStandardOutput();
    qDebug() << output;

    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(output, &parseError);

    if (parseError.error == QJsonParseError::NoError) {
        QJsonObject json = jsonDoc.object();

        QString accessToken = json.value("access_token").toString();
        QString refreshToken = json.value("refresh_token").toString();
        int expiresIn = json.value("expires_in").toInt();

        if (!accessToken.isEmpty()) {
            qDebug() << "Access Token:" << accessToken;
            qDebug() << "Refresh Token:" << refreshToken;
            qDebug() << "Expires in:" << expiresIn << "seconds";
            // Сохрани accessToken в QtKeychain или куда нужно

            QKeychain::WritePasswordJob *job = new QKeychain::WritePasswordJob(
                QCoreApplication::applicationName());
            job->setKey(kTokenKeyName);
            job->setTextData(accessToken);
            connect(job, &QKeychain::WritePasswordJob::finished, this, [job]() {
                if (job->error()) {
                    qWarning() << "Failed to write token:" << job->errorString();
                } else {
                    qDebug() << "Token saved to keychain!";
                }
                job->deleteLater();
            });
            job->start();
        } else {
            qWarning() << "access_token not found!";
        }
    } else {
        qWarning() << "JSON parsing failed:" << parseError.errorString();
    }
}

void MainWindow::configureProxy()
{
    // QNetworkProxyFactory::setUseSystemConfiguration(true);
    QSettings settings;
    QNetworkProxy proxy;
    proxy.setType(QNetworkProxy::HttpProxy);
    proxy.setHostName(settings.value("Proxy/host").toString());
    proxy.setPort(settings.value("Proxy/port").toInt());
    QNetworkProxy::setApplicationProxy(proxy);
}
