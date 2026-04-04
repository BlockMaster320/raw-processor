#pragma once

#include "image.h"

#include <QOpenGLBuffer>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>

#include <memory>

class ImageProcessor : protected QOpenGLFunctions {
public:
    ImageProcessor();
    ~ImageProcessor();

    void initializeGL();
    void cleanupGL();
    void processImage(std::shared_ptr<Image> image);

    GLuint getProcessedTexture() const;
    int getProcessedWidth() const;
    int getProcessedHeight() const;

    void setExposure(float exposure);
    float getExposure() const;

private:
    void uploadImage(std::shared_ptr<Image> image);
    void setupFbos(int width, int height);
    void renderRawToRgbPass();
    void renderAdjustmentPass();

    QOpenGLShaderProgram rawToRGBProgram;
    QOpenGLShaderProgram adjustmentProgram;

    QOpenGLBuffer vbo;
    QOpenGLVertexArrayObject vaoRawToRGB;
    QOpenGLVertexArrayObject vaoAdjustment;

    QOpenGLFramebufferObject* fboRawToRGB;
    QOpenGLFramebufferObject* fboAdjustment;
    QOpenGLTexture* rawTexture;

    int textureWidth;
    int textureHeight;
    float exposure;

    bool initialized;
    bool rawBitDirty;
    bool adjustmentBitDirty;

    std::shared_ptr<Image> currentImage;
};
