#ifndef CAMERANODE_H
#define CAMERANODE_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <QTcpServer>
#include <QTcpSocket>

class CameraNode : public QObject
{
	Q_OBJECT
public:
	explicit CameraNode(QObject* parent = nullptr);

public slots:
	void takeSnapshot();
	void handleNewConnection();
	void handleReadyRead();
	void handleDisconnected();

private:
	QTimer* m_timer;
	QString m_storagePath;

	QTcpServer* m_tcpServer;
	QTcpSocket* m_clientSocket;
	bool m_isAuthenticated = false;
	QString m_currentNonce;
	const QString m_expectedHash = "a78461df8c38a0174c8fb5340156b1abdf4459d3c2d2e3bc8d9923c67a336037";
};

#endif // CAMERANODE_H
