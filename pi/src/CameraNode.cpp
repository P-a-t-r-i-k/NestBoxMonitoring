#include "CameraNode.hpp"

#include <QProcess>
#include <QDateTime>
#include <QDir>
#include <QDebug>
#include <QRandomGenerator>
#include <QCryptographicHash>

CameraNode::CameraNode(QObject* parent) : QObject(parent)
{
	// Ensure the storage directory exists
	m_storagePath = QDir::homePath() + "/nest_box_images";
	QDir().mkpath(m_storagePath);

	m_timer = new QTimer(this);
	connect(m_timer, &QTimer::timeout, this, &CameraNode::takeSnapshot);

	qDebug() << "CameraNode started. Photos will be saved to: " << m_storagePath;

	// Take the first photo immediately
	takeSnapshot();

	// Start the repeating timer
	int deltaTimeMs = 1800000;
	m_timer->start(deltaTimeMs);

	// Initialize TCP server on port 5000
	m_clientSocket = nullptr;
	m_tcpServer = new QTcpServer(this);
	connect(m_tcpServer, &QTcpServer::newConnection, this, &CameraNode::handleNewConnection);

	if (m_tcpServer->listen(QHostAddress::AnyIPv4, 5000))
		qDebug() << "Server listening on port 5000. Ready for secure connections.";
}

void CameraNode::handleNewConnection()
{
	if (m_clientSocket)
	{
		// Allow only one client at a time
		QTcpSocket* extraSocket = m_tcpServer->nextPendingConnection();
		extraSocket->disconnectFromHost();
		return;
	}

	m_clientSocket = m_tcpServer->nextPendingConnection();
	m_isAuthenticated = false;

	// Generate a random nonce
	m_currentNonce = QString::number(QRandomGenerator::global()->generate());
	
	connect(m_clientSocket, &QTcpSocket::readyRead, this, &CameraNode::handleReadyRead);
	connect(m_clientSocket, &QTcpSocket::disconnected, this, &CameraNode::handleDisconnected);

	m_clientSocket->write(m_currentNonce.toUtf8());
	qDebug() << "Client connected. Sent nonce: " << m_currentNonce;
}

void CameraNode::handleReadyRead()
{
	if (!m_clientSocket)
		return;

	QByteArray data = m_clientSocket->readAll().trimmed();

	if (!m_isAuthenticated)
	{
		// Verify the response
		QByteArray combined = QByteArray::fromHex(m_expectedHash.toUtf8());
		combined.append(m_currentNonce.toUtf8());

		QByteArray expectedResponse = QCryptographicHash::hash(combined, QCryptographicHash::Sha256).toHex();
		
		if (data == expectedResponse)
		{
			m_isAuthenticated = true;
			qDebug() << "AUTH_SUCCESS";
			m_clientSocket->write("AUTH_SUCCESS\n");
		}
		else
		{
			qDebug() << "AUTH_FAIL";
			m_clientSocket->write("AUTH_FAIL\n");
			m_clientSocket->disconnectFromHost();
		}

		return;
	}

	QString request = QString::fromUtf8(data);
	QStringList parts = request.split("-");
	int totalNeededParts = 2;
	if (parts.size() == totalNeededParts && parts[0].toLower() == "list")
	{
		QString date = parts[1];
		QString dailyFolder = m_storagePath + "/" + date;

		qDebug() << "Client requested image list for date: " << date;

		// Get content from the requested folder
		QDir dir(dailyFolder);
		if (!dir.exists())
		{
			m_clientSocket->write("EMPTY\n");
			return;
		}

		QStringList filters;
		filters << "*.jpg";
		QStringList files = dir.entryList(filters, QDir::Files, QDir::Name);

		if (files.size() == 0)
		{
			m_clientSocket->write("EMPTY\n");
                        return;
		}

		QByteArray response = files.join(",").toUtf8();

		m_clientSocket->write(response + "\n");
		m_clientSocket->flush();

		qDebug() << "Sent" << files.size() << "filenames to client.";
	}
	else if (parts.size() == totalNeededParts && parts[0].toLower() == "get")
	{
		sendImages(parts[1]);
	}
}

void CameraNode::handleDisconnected()
{
	qDebug() << "Client disconnected.";
	m_clientSocket = nullptr;
	m_isAuthenticated = false;
}

void CameraNode::takeSnapshot()
{
	QDateTime now = QDateTime::currentDateTime();
	QString dateFolder = now.toString("yyyyMMdd");
	QString timestamp = now.toString("yyyyMMdd_hhmmss");

	QString dailyFilePath = m_storagePath + '/' + dateFolder;

	// Ensure the daily folder exists
	QDir().mkpath(dailyFilePath);

	QString filePath = QString("%1/img_%2.jpg").arg(dailyFilePath, timestamp);

	qDebug() << "Triggering snapshot, target file: " << filePath;

	// Camera settings
	QStringList args;
	args << "-o" << filePath
	     << "--shutter" << "500000"
	     << "--gain" << "6"
	     << "--contrast" << "1.4"
	     << "--saturation" << "0"
	     << "--immediate";

	// QProcess runs the external rpicam-still command
	QProcess* cameraProcess = new QProcess(this);
	cameraProcess->start("rpicam-still", args);

	if (cameraProcess->waitForFinished(10000))
		qDebug() << "Successfully saved: " << filePath;
	else
		qCritical() << "Camera process timed out or failed!";
	
	cameraProcess->deleteLater();
}

void CameraNode::sendImages(QString imagesString)
{
	QStringList images = imagesString.split(",");
	qDebug() << "Total images =" << images.size();
	for (const QString& image : images)
	{
		QStringList parts = image.split("_");
		if (parts.size() != 3)
			continue;

		QString date = parts[1];
		QString fullPath = m_storagePath + "/" + date + "/" + image;
		qDebug() << fullPath;
		QFile file(fullPath);

		if (file.open(QIODevice::ReadOnly))
		{
			int size = file.size();
			QByteArray header = QString("info-%1-%2\n").arg(image).arg(size).toUtf8();

			qDebug() << "Image:" << image;
			qDebug() << "Sending header...";
			m_clientSocket->write(header);

			qDebug() << "Sending data...";
			m_clientSocket->write(file.readAll());
			m_clientSocket->flush();

			file.close();
		}
	}
}
