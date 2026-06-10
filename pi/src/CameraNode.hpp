#ifndef CAMERANODE_H
#define CAMERANODE_H

#include <QObject>
#include <QString>
#include <QTimer>

class CameraNode : public QObject
{
	Q_OBJECT
public:
	explicit CameraNode(QObject* parent = nullptr);

public slots:
	void takeSnapshot();

private:
	QTimer* m_timer;
	QString m_storagePath;
};

#endif // CAMERANODE_H
