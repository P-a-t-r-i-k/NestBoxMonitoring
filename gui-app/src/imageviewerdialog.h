#ifndef IMAGEVIEWERDIALOG_H
#define IMAGEVIEWERDIALOG_H

#include "mainwindow.h"
#include <QDialog>
#include <QTcpSocket>

namespace Ui {
class ImageViewerDialog;
}

class ImageViewerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ImageViewerDialog(QTcpSocket* tcpSocket, MainWindow::ConnectionState connectionState, QWidget* parent = nullptr);
    ~ImageViewerDialog();

private:
    Ui::ImageViewerDialog* m_ui;
    QTcpSocket* m_tcpSocket;
    QString m_selectedDirectoryPath = "";

    int m_expectedBytes = 0;
    QString m_downloadingFileName;
    QByteArray m_imageBuffer;
    MainWindow::ConnectionState m_currentConnectionState;

private slots:
    void onListButtonClicked();
    void onSocketReadyRead();
    void onOpenFolderButtonClicked();
    void onShowImageButtonClicked();
    void onDownloadButtonClicked();

private:
    void updateDownloadDirContent();
    void processDownloadedImage();
};

#endif // IMAGEVIEWERDIALOG_H
