#pragma once

#include "image.h"

#include <QOpenGLBuffer>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>

#include <memory>
#include <string>
#include <utility>
#include <unordered_map>

struct GlobalAdjUniforms {
    float exposure = 0.0f;
    float contrast = 0.0f;
    float midpoint = 0.0f;
    float popArt = 0.0f;
    float white = 0.0f;
    float black = 0.0f;
    float saturation = 0.0f;
};

class ImageProcessor : protected QOpenGLFunctions {
public:
    enum class FilterPassType {
        RgbToYcbcr,
        GaussianChroma,
        BilateralLuma,
        YcbcrToRgb,
    };

    struct FilterAdjUniforms {
        FilterAdjUniforms& setFloat(const std::string& name, float value);  // sets a float uniform
        FilterAdjUniforms& setInt(const std::string& name, int value);    // sets an int uniform
        std::unordered_map<std::string, float> floatUniforms;
        std::unordered_map<std::string, int> intUniforms;
    };

    ImageProcessor();
    ~ImageProcessor();

    void initializeGL();
    void cleanupGL();
    void processImage(std::shared_ptr<Image> image);

    GLuint getProcessedTexture() const;
    int getProcessedWidth() const;
    int getProcessedHeight() const;

    void markAdjustmentDirty();

    void renderGlobalAdjustments();
    void renderFilterPass(FilterPassType passType, const FilterAdjUniforms& uniforms = FilterAdjUniforms{});

    void swapFbos();

    GlobalAdjUniforms uniforms;
    bool uniformsDirty = false;

private:
    void uploadImage(std::shared_ptr<Image> image);
    void setupFbos(int width, int height);
    void renderRawToRgbPass();
    void renderAdjustmentPass();
    void renderPostprocessPass();
    void renderGenericPass(QOpenGLShaderProgram& program, const FilterAdjUniforms* localUniforms = nullptr);

    void setUniforms();
    void resetUniforms();

    QOpenGLShaderProgram rawToRGBProgram;
    QOpenGLShaderProgram adjustmentProgram;
    QOpenGLShaderProgram rgbToYcbcrProgram;
    QOpenGLShaderProgram gaussianChromaProgram;
    QOpenGLShaderProgram bilateralLumaProgram;
    QOpenGLShaderProgram ycbcrToRgbProgram;
    QOpenGLShaderProgram postprocessProgram;

    QOpenGLBuffer vbo;
    QOpenGLVertexArrayObject vaoRawToRGB;
    QOpenGLVertexArrayObject vaoAdjustment;

    QOpenGLFramebufferObject* fboRgb;
    QOpenGLFramebufferObject* fboAdjustment1;
    QOpenGLFramebufferObject* fboAdjustment2;
    QOpenGLFramebufferObject* fboFree;
    QOpenGLFramebufferObject* fboOccupied;

    bool isFboRgbUsed;
    //bool fboSwap;

    QOpenGLTexture* rawTexture;

    int textureWidth;
    int textureHeight;

    bool initialized;
    bool rawBitDirty;
    bool adjustmentBitDirty;

    std::shared_ptr<Image> currentImage;
};
