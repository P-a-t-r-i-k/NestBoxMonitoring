#include "imageviewerdialog.h"
#include "ui_imageviewerdialog.h"
#include <QFileDialog>
#include <QMessageBox>

ImageViewerDialog::ImageViewerDialog(QTcpSocket* tcpSocket, MainWindow::ConnectionState connectionState, QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui::ImageViewerDialog)
    , m_tcpSocket(tcpSocket)
    , m_currentConnectionState(connectionState)
{
    m_ui->setupUi(this);

    connect(m_ui->listButton, &QPushButton::clicked, this, &ImageViewerDialog::onListButtonClicked);
    connect(m_tcpSocket, &QTcpSocket::readyRead, this, &ImageViewerDialog::onSocketReadyRead);
    connect(m_ui->openFolderButton, &QPushButton::clicked, this, &ImageViewerDialog::onOpenFolderButtonClicked);
    connect(m_ui->showImageButton, &QPushButton::clicked, this, &ImageViewerDialog::onShowImageButtonClicked);
    connect(m_ui->downloadImagesButton, &QPushButton::clicked, this, &ImageViewerDialog::onDownloadButtonClicked);
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
    while (m_tcpSocket->bytesAvailable() > 0)
    {
        if (m_currentConnectionState == MainWindow::ConnectionState::Authenticated)
        {
            if (!m_tcpSocket->canReadLine())
                return;

            QByteArray line = m_tcpSocket->readLine().trimmed();
            QString response = QString::fromUtf8(line);

            if (response.startsWith("info-"))
            {
                QStringList parts = response.split('-');
                if (parts.size() == 3)
                {
                    m_downloadingFileName = parts[1];
                    m_expectedBytes = parts[2].toLongLong();
                    m_imageBuffer.clear();
                    m_currentConnectionState = MainWindow::ConnectionState::DownloadingImage;
                    qDebug() << "Incoming image:" << m_downloadingFileName << "Size:" << m_expectedBytes;
                }
            }
            else if (response.toLower() == "empty")
            {
                QListWidgetItem* item = new QListWidgetItem("No images for this day", m_ui->availableImagesListWidget);
                item->setTextAlignment(Qt::AlignCenter);
                m_ui->availableImagesListWidget->setSelectionMode(QAbstractItemView::NoSelection);
                return;
            }
            else
            {
                m_ui->availableImagesListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
                QStringList imageNames = response.split(',');
                for (const QString& name : std::as_const(imageNames))
                {
                    QListWidgetItem* item = new QListWidgetItem(name, m_ui->availableImagesListWidget);
                    item->setTextAlignment(Qt::AlignCenter);
                }
            }
        }
        else if (m_currentConnectionState == MainWindow::ConnectionState::DownloadingImage)
        {
            int bytesToRead = qMin(m_tcpSocket->bytesAvailable(), m_expectedBytes - m_imageBuffer.size());
            m_imageBuffer.append(m_tcpSocket->read(bytesToRead));

            if (m_imageBuffer.size() == m_expectedBytes)
            {
                // Image fully received
                processDownloadedImage();
                m_currentConnectionState = MainWindow::ConnectionState::Authenticated;
            }
        }
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

void ImageViewerDialog::onShowImageButtonClicked()
{
    QListWidgetItem* selectedItem = m_ui->downloadedImagesListWidget->currentItem();
    if (!selectedItem)
    {
        QMessageBox::critical(this, "Image Error", "An error occurred while loading the selected image!");
        return;
    }

    QString filename = selectedItem->text();
    QString filepath = m_selectedDirectoryPath + "/" + filename;
    qDebug() << "User wants to show:" << filepath;

    QPixmap pix(filepath);
    if (!pix.isNull())
    {
        m_ui->imageLabel->setText("");
        m_ui->imageLabel->setPixmap(pix.scaled(m_ui->imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        m_ui->imageLabel->setFrameShape(QFrame::NoFrame);

        QStringList parts = filename.split(".")[0].split("_");
        if (parts[0] == "img" && parts.size() == 3)
        {
            QString date = parts[1];
            QDate qDate = QDate::fromString(date, "yyyyMMdd");
            m_ui->dateLabel->setText(qDate.toString("dd.MM.yyyy"));

            QString time = parts[2];
            QTime qTime = QTime::fromString(time, "hhmmss");
            m_ui->timeLabel->setText(qTime.toString("hh:mm:ss"));
        }
    }
    else
    {
        QMessageBox::critical(this, "Image Error", "Image could not be found!");
        m_ui->imageLabel->setText("No image to show");
        m_ui->imageLabel->setFrameShape(QFrame::Box);
    }
}

void ImageViewerDialog::onDownloadButtonClicked()
{
    QList<QListWidgetItem*> selectedItems = m_ui->availableImagesListWidget->selectedItems();
    if (selectedItems.count() == 0)
        return;

    if (m_selectedDirectoryPath == "")
    {
        QMessageBox::critical(this, "Download folder error", "No download folder selected!");
        return;
    }

    QString imagesToDownload;
    int i = 0;
    for (const auto& selectedItem : std::as_const(selectedItems))
    {
        imagesToDownload = imagesToDownload % selectedItem->text();
        if (i < selectedItems.count() - 1)
            imagesToDownload = imagesToDownload % ",";

        ++i;
    }

    m_tcpSocket->write("get-" + imagesToDownload.toUtf8());

    QStringList images = imagesToDownload.split(",");
    qDebug() << "Total images =" << images.size();

    qDebug() << "Send request for images" << imagesToDownload;
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

void ImageViewerDialog::processDownloadedImage()
{
    QString path = m_selectedDirectoryPath + "/" + m_downloadingFileName;
    QFile file(path);
    if (file.open(QIODevice::WriteOnly))
    {
        file.write(m_imageBuffer);
        file.close();
        qDebug() << "Image saved to:" << path;

        // Update list widget for downloaded images
        QListWidgetItem* item = new QListWidgetItem(m_downloadingFileName, m_ui->downloadedImagesListWidget);
        item->setTextAlignment(Qt::AlignCenter);
    }
    else
    {
        qCritical() << "Failed to save image to:" << path;
        QMessageBox::critical(this, "Image Error", "Image could not be saved to " + path);
        return;
    }
}
