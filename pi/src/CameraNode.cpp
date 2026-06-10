#include "CameraNode.hpp"

#include <QProcess>
#include <QDateTime>
#include <QDir>
#include <QDebug>

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
