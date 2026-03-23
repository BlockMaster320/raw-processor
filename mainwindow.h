#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "PieMenu.h"

#include <QWidget>
#include <QMainWindow>

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLbuffer>
#include <QOpenGLFramebufferObject>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>

#include <QPushButton>
#include <QProgressBar>
#include <QSlider>
#include <QRadioButton>
#include <QButtonGroup>
#include <QKeyEvent>

class Canvas : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    Canvas(QWidget *parent = nullptr);
    ~Canvas();

    virtual void initializeGL() override;
    virtual void resizeGL(int w, int h) override;
    virtual void paintGL() override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

    void updateTransform();
    void setCompareMode(int mode);

public slots:
    void onSliderChanged(int value);

private:
    QOpenGLShaderProgram rawToRGBProgram;
    QOpenGLShaderProgram adjustmentProgram;
    QOpenGLShaderProgram displayProgram;
    QOpenGLBuffer vbo;
    QOpenGLVertexArrayObject vaoRawToRGB;
    QOpenGLVertexArrayObject vaoAdjustment;
    QOpenGLVertexArrayObject vaoDisplay;
    QOpenGLFramebufferObject* fboRawToRGB;
    QOpenGLFramebufferObject* fboAdjustment;

    QOpenGLTexture* texture;
    QOpenGLTexture* referenceTexture;
    int textureWidth, textureHeight;
    bool compareWithLibRaw;
    int compareMode;

    bool isMouseDragging;
    QPoint mouseLastPos;
    QPointF imgOffset;
    float imgZoom;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent *event) override;

signals:
public slots:
    void onButtonClicked();

private:
    Canvas *canvas;

    QPushButton *btn;
    QProgressBar *pbar;
    QSlider *slider;
    QRadioButton *rBtn1;
    QRadioButton *rBtn2;
    QRadioButton *rBtn3;
    QButtonGroup *btnGroup;

    PieMenu *pieMenu;
};

#endif // MAINWINDOW_H