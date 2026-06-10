#include "CameraNode.hpp"

#include <QCoreApplication>

int main(int argc, char* argv[])
{
	QCoreApplication app(argc, argv);

	CameraNode cameraNode;

	return app.exec();
}
