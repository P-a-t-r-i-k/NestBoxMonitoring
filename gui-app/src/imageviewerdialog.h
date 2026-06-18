#ifndef IMAGEVIEWERDIALOG_H
#define IMAGEVIEWERDIALOG_H

#include <QDialog>
#include <QTcpSocket>

namespace Ui {
class ImageViewerDialog;
}

class ImageViewerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ImageViewerDialog(QTcpSocket* tcpSocket, QWidget *parent = nullptr);
    ~ImageViewerDialog();

private:
    Ui::ImageViewerDialog* m_ui;
    QTcpSocket* m_tcpSocket;

private slots:
    void onListButtonClicked();
    void onSocketReadyRead();
};

#endif // IMAGEVIEWERDIALOG_H
