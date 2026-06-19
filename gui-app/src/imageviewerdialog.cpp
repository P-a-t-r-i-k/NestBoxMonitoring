#include "imageviewerdialog.h"
#include "ui_imageviewerdialog.h"
#include <QFileDialog>
#include <QMessageBox>

ImageViewerDialog::ImageViewerDialog(QTcpSocket* tcpSocket, QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui::ImageViewerDialog)
    , m_tcpSocket(tcpSocket)
{
    m_ui->setupUi(this);

    connect(m_ui->listButton, &QPushButton::clicked, this, &ImageViewerDialog::onListButtonClicked);
    connect(m_tcpSocket, &QTcpSocket::readyRead, this, &ImageViewerDialog::onSocketReadyRead);
    connect(m_ui->openFolderButton, &QPushButton::clicked, this, &ImageViewerDialog::onOpenFolderButtonClicked);
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
    m_ui->availableImagesListWidget->clear();
    QByteArray data = m_tcpSocket->readAll().trimmed();
    QString response = QString::fromUtf8(data);

    if (response.toLower() == "empty")
    {
        QListWidgetItem* item = new QListWidgetItem("No images for this day", m_ui->availableImagesListWidget);
        item->setTextAlignment(Qt::AlignCenter);
        m_ui->availableImagesListWidget->setSelectionMode(QAbstractItemView::NoSelection);
        return;
    }

    m_ui->availableImagesListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    QStringList imageNames = response.split(',');
    for (const QString& name : std::as_const(imageNames))
    {
        QListWidgetItem* item = new QListWidgetItem(name, m_ui->availableImagesListWidget);
        item->setTextAlignment(Qt::AlignCenter);
    }
}

void ImageViewerDialog::onOpenFolderButtonClicked()
{
    QString directoryPath = QFileDialog::getExistingDirectory(this, "Select download directory", QDir::homePath(), QFileDialog::ShowDirsOnly);

    if (!directoryPath.isEmpty())
    {
        qDebug() << "User selected folder:" << directoryPath;
        m_selectedDirectoryPath = directoryPath;
        updateDownloadDirContent();
        m_ui->downloadedImagesLabel->setToolTip("Download folder: " + m_selectedDirectoryPath);
    }
    else
        QMessageBox::information(this, "Warning", "No folder has been selected.");
}

void ImageViewerDialog::updateDownloadDirContent()
{
    QDir directory(m_selectedDirectoryPath);
    QStringList filters;
    filters << "*.jpg" << "*.jpeg";

    // Do not pick subfolders and sort files alphabetically
    QStringList images = directory.entryList(filters, QDir::Files, QDir::Name);
    m_ui->downloadedImagesListWidget->clear();

    for (const QString& image : std::as_const(images))
    {
        QListWidgetItem* item = new QListWidgetItem(image, m_ui->downloadedImagesListWidget);
        item->setTextAlignment(Qt::AlignCenter);
    }
}
