#pragma once

#include "../image.h"
#include "../imageprocessor.h"

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>

class ImageViewer : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    ImageViewer(QWidget *parent = nullptr);
    ~ImageViewer();

    virtual void initializeGL() override;
    virtual void resizeGL(int w, int h) override;
    virtual void paintGL() override;

    void setImage(std::shared_ptr<Image> image);

    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

    void updateTransform();
    void setCompareMode(int mode);

public slots:
    void onAdjustmentChanged();

private:
    QOpenGLShaderProgram displayProgram;
    QOpenGLBuffer vbo;
    QOpenGLVertexArrayObject vaoDisplay;

    std::shared_ptr<Image> image;
    ImageProcessor imageProcessor;

    // Libraw-processed reference image for comparison
    QOpenGLTexture* referenceTexture;
    int textureWidth, textureHeight;
    bool compareWithLibRaw;
    int compareMode;

    // Viewport interaction
    bool isMouseDragging;
    QPoint mouseLastPos;
    QPointF imgOffset;
    float imgZoom;
};