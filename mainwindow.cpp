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
    auto job = new QKeychain::ReadPasswordJob(QCoreApplication::applicationName(), this);
    job->setKey("CopernicusAccessToken");
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
    QProcess which;
    which.start("where", QStringList() << "curl");
    which.waitForFinished();
    qDebug() << "curl path:" << which.readAllStandardOutput();
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
                "Notus");                         // имя сервиса, можно любое
            job->setKey("CopernicusAccessToken"); // ключ, под которым сохраняется
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

    // const QString username
    //     = "gumben7@gmail.com"; //QInputDialog::getText(this, "Copernicus Login", "Username:");
    // const QString password = "vhR5DT!rpw8Bb.f";
    // // = QInputDialog::getText(this, "Copernicus Password", "Password:", QLineEdit::Password);
    // if (username.isEmpty() || password.isEmpty()) {
    //     qWarning() << "Username or password was empty, aborting token request.";
    //     return;
    // }

    // Подготовка данных
    // QByteArray postData;
    // postData.append("username=").append(QUrl::toPercentEncoding(username));
    // postData.append("&password=").append(QUrl::toPercentEncoding(password));
    // postData.append("&grant_type=password");
    // postData.append("&client_id=cdse-public");

    // QUrl url(
    //     "https://identity.dataspace.copernicus.eu/auth/realms/CDSE/protocol/openid-connect/token");
    // QNetworkRequest request(url);

    // // МАКСИМАЛЬНО точно копируем curl-заголовки
    // request.setRawHeader("Host", "identity.dataspace.copernicus.eu");
    // request.setRawHeader("User-Agent", "curl/8.11.1");
    // request.setRawHeader("Accept", "*/*");
    // request.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
    // request.setRawHeader("Content-Length", QByteArray::number(postData.size()));
    // request.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);

    // qDebug() << "Sending POST to" << url;
    // qDebug() << "Payload:" << postData;

    // QNetworkReply *reply = networkManager->post(request, postData);

    // connect(reply, &QNetworkReply::finished, this, [=]() {
    //     QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    //     if (statusCode.isValid()) {
    //         qDebug() << "HTTP Status Code:" << statusCode.toInt();
    //     }
    //     qDebug() << "Raw headers:";
    //     const auto headers = reply->rawHeaderPairs();
    //     for (const auto &pair : headers) {
    //         qDebug() << pair.first << ":" << pair.second;
    //     }
    //     QByteArray responseData = reply->readAll();
    //     qDebug() << "Reply error:" << reply->errorString();
    //     qDebug() << "Full reply:" << responseData;

    //     if (reply->error() == QNetworkReply::NoError) {
    //         QJsonParseError parseError;
    //         QJsonDocument jsonResponse = QJsonDocument::fromJson(responseData, &parseError);
    //         if (parseError.error == QJsonParseError::NoError) {
    //             QJsonObject responseObject = jsonResponse.object();
    //             QString accessToken = responseObject.value("access_token").toString();
    //             if (!accessToken.isEmpty()) {
    //                 qDebug() << "Access Token:" << accessToken;
    //                 // тут можно сохранить
    //             } else {
    //                 qWarning() << "No access_token in response.";
    //             }
    //         } else {
    //             qWarning() << "JSON Parse Error:" << parseError.errorString();
    //         }
    //     } else {
    //         qWarning() << "Token request failed:" << reply->errorString();
    //     }

    //     reply->deleteLater();
    // });
}

void MainWindow::configureProxy()
{
    // QNetworkProxyFactory::setUseSystemConfiguration(true);
    QSettings settings;
    QNetworkProxy proxy;
    proxy.setType(QNetworkProxy::HttpProxy);
    proxy.setHostName(settings.value("Proxy/host").toString());
    proxy.setPort(settings.value("Proxy/port").toInt());
    QNetworkProxy::setApplicationProxy(proxy); //(QNetworkProxy::NoProxy); //
}
