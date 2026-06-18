#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "imageviewerdialog.h"

#include <QPushButton>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_ui(new Ui::MainWindow)
{
    m_ui->setupUi(this);
    m_ui->disconnectButton->setEnabled(false);
    m_ui->goToImagesButton->setEnabled(false);

    connect(m_ui->connectButton, &QPushButton::clicked, this, &MainWindow::onConnectButtonClicked);
    connect(m_ui->disconnectButton, &QPushButton::clicked, this, &MainWindow::handleDisconnection);
    connect(m_ui->goToImagesButton, &QPushButton::clicked, this, &MainWindow::onGoToImagesButtonClicked);

    // Initialize the socket
    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::connected, this, &MainWindow::onSocketConnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &MainWindow::onSocketReadyRead);
    m_currentConnectionState = ConnectionState::Idle;
    m_ui->statusLabel->setText("Idle");
}

MainWindow::~MainWindow()
{
    delete m_ui;
}

void MainWindow::onConnectButtonClicked()
{
    QString ipAddress = m_ui->ipAddressLineEdit->text();
    int port = m_ui->portLineEdit->text().toInt();
    m_socket->connectToHost(ipAddress, port);
    m_ui->statusLabel->setStyleSheet("color: black;");
    m_ui->statusLabel->setText("Connecting...");
    m_currentConnectionState = ConnectionState::WaitingForNonce;
}

void MainWindow::onSocketReadyRead()
{
    QByteArray data = m_socket->readAll().trimmed();

    switch (m_currentConnectionState)
    {
        case ConnectionState::WaitingForNonce:
            sendResponseFromNonce(data);
            m_currentConnectionState = ConnectionState::WaitingForAuthResult;
            m_ui->connectButton->setEnabled(false);
            break;

        case ConnectionState::WaitingForAuthResult:
            if (data == "AUTH_SUCCESS")
                handleSuccessfullAuthentication();
            else
                handleFailedAuthentication();
            break;

        case ConnectionState::Authenticated:
            handleIncomingData(data);
            break;
    }
}

void MainWindow::onSocketConnected()
{
    m_ui->statusLabel->setText("Waiting for nonce...");
}

void MainWindow::onGoToImagesButtonClicked()
{
    disconnect(m_socket, &QTcpSocket::readyRead, this, &MainWindow::onSocketReadyRead);

    ImageViewerDialog imageViewerDialog(m_socket);
    imageViewerDialog.exec();

    connect(m_socket, &QTcpSocket::readyRead, this, &MainWindow::onSocketReadyRead);
}

void MainWindow::sendResponseFromNonce(QByteArray nonce)
{
    qDebug() << "Received nonce:" << nonce;

    QByteArray hashedPassword = QCryptographicHash::hash(m_ui->passwordLineEdit->text().toUtf8(), QCryptographicHash::Sha256);
    QByteArray response = QCryptographicHash::hash(hashedPassword + nonce, QCryptographicHash::Sha256).toHex();

    // Send the response back to the Pi
    m_socket->write(response);
}

void MainWindow::handleIncomingData(QByteArray data)
{
    qDebug() << "Not implemented yet!";
}

void MainWindow::handleSuccessfullAuthentication()
{
    m_currentConnectionState = ConnectionState::Authenticated;
    m_ui->statusLabel->setStyleSheet("color: #28a745;");
    m_ui->statusLabel->setText("Connected and authenticated");
    m_ui->connectButton->setEnabled(false);
    m_ui->disconnectButton->setEnabled(true);
    m_ui->goToImagesButton->setEnabled(true);
}

void MainWindow::handleFailedAuthentication()
{
    handleDisconnection();
    m_ui->statusLabel->setStyleSheet("color: #dc3545;");
    m_ui->statusLabel->setText("Authentication failed!");
}

void MainWindow::handleDisconnection()
{
    m_socket->disconnectFromHost();
    m_currentConnectionState = ConnectionState::Idle;
    m_ui->statusLabel->setStyleSheet("color: black;");
    m_ui->statusLabel->setText("Idle");
    m_ui->connectButton->setEnabled(true);
    m_ui->disconnectButton->setEnabled(false);
    m_ui->goToImagesButton->setEnabled(false);
}
