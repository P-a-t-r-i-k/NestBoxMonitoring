#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QCryptographicHash>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    enum class ConnectionState
    {
        Idle,
        WaitingForNonce,
        WaitingForAuthResult,
        Authenticated,
        DownloadingImage
    };

    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onConnectButtonClicked();
    void onSocketReadyRead();
    void onSocketConnected();
    void onGoToImagesButtonClicked();

private:
    void sendResponseFromNonce(QByteArray nonce);
    void handleIncomingData(QByteArray data);
    void handleSuccessfullAuthentication();
    void handleFailedAuthentication();
    void handleDisconnection();

private:
    Ui::MainWindow* m_ui;
    QTcpSocket* m_socket;
    ConnectionState m_currentConnectionState;
};
#endif // MAINWINDOW_H
