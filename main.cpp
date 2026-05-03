#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "ui/mainwindow.h"

#include <QApplication>
#include <QPushButton>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Set OpenGL surface format
    QSurfaceFormat format;
        format.setVersion(3,3);
        format.setProfile(QSurfaceFormat::CoreProfile);
        format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
        format.setRedBufferSize(8);
        format.setGreenBufferSize(8);
        format.setBlueBufferSize(8);
        format.setAlphaBufferSize(8);
        //format.setColorSpace(QSurfaceFormat::sRGBColorSpace);
    QSurfaceFormat::setDefaultFormat(format);

    MainWindow window;



    window.show();
    return app.exec();
}