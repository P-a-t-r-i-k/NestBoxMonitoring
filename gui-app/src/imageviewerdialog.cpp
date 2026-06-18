#include "imageviewerdialog.h"
#include "ui_imageviewerdialog.h"

ImageViewerDialog::ImageViewerDialog(QTcpSocket* tcpSocket, QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui::ImageViewerDialog)
    , m_tcpSocket(tcpSocket)
{
    m_ui->setupUi(this);

    connect(m_ui->listButton, &QPushButton::clicked, this, &ImageViewerDialog::onListButtonClicked);
    connect(m_tcpSocket, &QTcpSocket::readyRead, this, &ImageViewerDialog::onSocketReadyRead);
}

ImageViewerDialog::~ImageViewerDialog()
{
    delete m_ui;
}

void ImageViewerDialog::onListButtonClicked()
{
    int year, month, day;
    m_ui->calendarWidget->selectedDate().getDate(&year, &month, &day);

    QString dateString = QString::asprintf("%4d%02d%02d", year, month, day);
    m_tcpSocket->write("list-" + dateString.toUtf8());

    qDebug() << "Requesting list for:" << dateString;
}

void ImageViewerDialog::onSocketReadyRead()
{
    m_ui->imageListWidget->clear();
    QByteArray data = m_tcpSocket->readAll().trimmed();
    QString response = QString::fromUtf8(data);

    if (response.toLower() == "empty")
    {
        QListWidgetItem* item = new QListWidgetItem("No images for this day", m_ui->imageListWidget);
        item->setTextAlignment(Qt::AlignCenter);
        m_ui->imageListWidget->setSelectionMode(QAbstractItemView::NoSelection);
        return;
    }

    m_ui->imageListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    QStringList imageNames = response.split(',');
    for (const QString& name : std::as_const(imageNames))
    {
        QListWidgetItem* item = new QListWidgetItem(name, m_ui->imageListWidget);
        item->setTextAlignment(Qt::AlignCenter);
    }

    //m_ui->imageListWidget->addItems(imageNames);
}
