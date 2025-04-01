#include "mainwindow.h"
#include "ui_mainwindow.h"

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
    QUrl url("https://dataspace.copernicus.eu"); //"https://identity.dataspace.copernicus.eu");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "Qt App");
    networkManager->get(request);
}

void MainWindow::dataspaceReplyFinished(QNetworkReply *reply)
{
    int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QString responseBody = reply->readAll();
    QPalette palette = ui->dataspaceStatusLabel->palette();
    if (reply->error() == QNetworkReply::NoError) {
        if (httpStatus == 200 && responseBody.contains("The requested URL was rejected")) {
            ui->dataspaceStatusLabel->setText("dataspace.copernicus.eu: Доступ ограничен");
            palette.setColor(QPalette::WindowText, QColor(255, 165, 0));
        } else {
            ui->dataspaceStatusLabel->setText("dataspace.copernicus.eu: Доступен");
            palette.setColor(QPalette::WindowText, Qt::green);
        }
    } else {
        ui->dataspaceStatusLabel->setText("dataspace.copernicus.eu: Недоступен ("
                                          + reply->errorString() + ")");
        palette.setColor(QPalette::WindowText, Qt::red);
    }
    ui->dataspaceStatusLabel->setStyleSheet("");
    ui->dataspaceStatusLabel->setPalette(palette);
    reply->deleteLater();
}

void MainWindow::configureProxy()
{
    QNetworkProxy proxy;
    proxy.setType(QNetworkProxy::HttpProxy);
    proxy.setHostName("54.37.207.54");
    proxy.setPort(3128);
    QNetworkProxy::setApplicationProxy(proxy);
}
