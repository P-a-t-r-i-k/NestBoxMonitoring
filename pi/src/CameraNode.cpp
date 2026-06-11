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
}

void CameraNode::handleDisconnected()
{
	qDebug() << "Client disconnected.";
	m_clientSocket = nullptr;
	m_isAuthenticated = false;
}

void CameraNode::takeSnapshot()
{
	QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
	QString filePath = QString("%1/img_%2.jpg").arg(m_storagePath, timestamp);

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
