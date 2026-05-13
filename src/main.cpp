#define STB_IMAGE_IMPLEMENTATION
#include "core/stb_image.h"

#include "ui/mainwindow.h"

#include <QApplication>
#include <QPushButton>
#include <QDebug>
#include <QFontDatabase>
#include <QPalette>
#include <QColor>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Set up fonts
    QFontDatabase::addApplicationFont(":/resources/fonts/Inter-Regular.ttf");
    QFontDatabase::addApplicationFont(":/resources/fonts/Inter-Medium.ttf");
    QFontDatabase::addApplicationFont(":/resources/fonts/Inter-Bold.ttf");

    QFont font("Inter");
    font.setPointSize(10);

    app.setFont(font);

    // Force a dark palette so text colors are correct regardless of the Windows light/dark mode setting.
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window,          QColor("#272727"));
    darkPalette.setColor(QPalette::WindowText,      QColor("#dfe4ea"));
    darkPalette.setColor(QPalette::Base,            QColor("#1b1b1b"));
    darkPalette.setColor(QPalette::AlternateBase,   QColor("#272727"));
    darkPalette.setColor(QPalette::Text,            QColor("#dfe4ea"));
    darkPalette.setColor(QPalette::ButtonText,      QColor("#dfe4ea"));
    darkPalette.setColor(QPalette::Button,          QColor("#414141"));
    darkPalette.setColor(QPalette::Highlight,       QColor("#5a9fd4"));
    darkPalette.setColor(QPalette::HighlightedText, QColor("#f1f2f6"));
    darkPalette.setColor(QPalette::ToolTipBase,     QColor("#414141"));
    darkPalette.setColor(QPalette::ToolTipText,     QColor("#dfe4ea"));
    darkPalette.setColor(QPalette::PlaceholderText, QColor("#6f7f8d"));
    app.setPalette(darkPalette);

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
