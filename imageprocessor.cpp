#include "imageprocessor.h"
#include "utility.h"

#include <QDebug>
#include <QOpenGLShader>

#include <algorithm>
#include <cmath>
#include <utility>

ImageProcessor::ImageProcessor()
    : rawToRGBProgram(), adjustmentProgram(), vbo(), vaoRawToRGB(), vaoAdjustment(),
      fboRawToRGB(nullptr), fboAdjustment(nullptr), rawTexture(nullptr),
      textureWidth(0), textureHeight(0), exposure(0.5f), initialized(false),
      rawBitDirty(true), adjustmentBitDirty(true), currentImage(nullptr)
{}

ImageProcessor::~ImageProcessor()
{
    cleanupGL();
}

void ImageProcessor::cleanupGL()
{
    delete rawTexture;
    rawTexture = nullptr;
    delete fboRawToRGB;
    fboRawToRGB = nullptr;
    delete fboAdjustment;
    fboAdjustment = nullptr;

    vaoRawToRGB.destroy();
    vaoAdjustment.destroy();
    vbo.destroy();

    initialized = false;
}

// Initializes OpenGL resources, compiles shaders and sets up vertex attributes. Does nothing if already initialized.
void ImageProcessor::initializeGL()
{
    if (initialized)
        return;

    initializeOpenGLFunctions();

    const std::string vertexShaderSourceRawToRGB = shaderFileToString("./shaders/rawtorgb.vert");
    const std::string fragmentShaderSourceRawToRGB = shaderFileToString("./shaders/rawtorgb.frag");

    rawToRGBProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSourceRawToRGB.c_str());
    rawToRGBProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSourceRawToRGB.c_str());
    rawToRGBProgram.link();

    const std::string vertexShaderSourceAdjustment = shaderFileToString("./shaders/adjustment.vert");
    const std::string fragmentShaderSourceAdjustment = shaderFileToString("./shaders/adjustment.frag");

    adjustmentProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSourceAdjustment.c_str());
    adjustmentProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSourceAdjustment.c_str());
    adjustmentProgram.link();

    const float verts[] = {
        -1.f, -1.f, 0.f, 0.f,
         1.f, -1.f, 1.f, 0.f,
        -1.f,  1.f, 0.f, 1.f,
         1.f,  1.f, 1.f, 1.f
    };

    vbo.create();
    vbo.bind();
    vbo.allocate(verts, sizeof(verts));

    vaoRawToRGB.create();
    vaoRawToRGB.bind();

    rawToRGBProgram.bind();
    rawToRGBProgram.enableAttributeArray(0);
    rawToRGBProgram.setAttributeBuffer(0, GL_FLOAT, 0, 2, 4 * sizeof(GLfloat));
    rawToRGBProgram.enableAttributeArray(1);
    rawToRGBProgram.setAttributeBuffer(1, GL_FLOAT, 2 * sizeof(GLfloat), 2, 4 * sizeof(GLfloat));
    rawToRGBProgram.setUniformValue("imageTex", 0);
    rawToRGBProgram.setUniformValue("cfaOffset", 0, 0);
    rawToRGBProgram.release();

    vaoRawToRGB.release();

    vaoAdjustment.create();
    vaoAdjustment.bind();

    adjustmentProgram.bind();
    adjustmentProgram.enableAttributeArray(0);
    adjustmentProgram.setAttributeBuffer(0, GL_FLOAT, 0, 2, 4 * sizeof(GLfloat));
    adjustmentProgram.enableAttributeArray(1);
    adjustmentProgram.setAttributeBuffer(1, GL_FLOAT, 2 * sizeof(GLfloat), 2, 4 * sizeof(GLfloat));
    adjustmentProgram.setUniformValue("imageTex", 0);
    adjustmentProgram.release();

    vaoAdjustment.release();
    vbo.release();

    initialized = true;
}

// Main processing function. Uploads the loaded image to GPU (if it's a new image) and runs the shader passes to produce the final processed texture.
// If the image is unchanged and the passes are not marked dirty, does nothing.
void ImageProcessor::process(std::shared_ptr<Image> image)
{
    if (!initialized || !image || !image->getIsLoaded())
        return;

    if (currentImage.get() != image.get()) // new image, upload and mark passes dirty
        uploadImage(image);

    if (!rawTexture || !fboRawToRGB || !fboAdjustment || textureWidth <= 0 || textureHeight <= 0)
        return;

    if (rawBitDirty) {
        renderRawToRgbPass();
        rawBitDirty = false;
        adjustmentBitDirty = true;
    }

    if (adjustmentBitDirty) {
        renderAdjustmentPass();
        adjustmentBitDirty = false;
    }
}

GLuint ImageProcessor::getProcessedTexture() const { return fboAdjustment ? fboAdjustment->texture() : 0; }
int ImageProcessor::getProcessedWidth() const { return textureWidth; }
int ImageProcessor::getProcessedHeight() const { return textureHeight; }

void ImageProcessor::setExposure(float exposure)
{
    adjustmentProgram.bind();
    adjustmentProgram.setUniformValue("exposure", exposure);
    adjustmentProgram.release();
    adjustmentBitDirty = true;
}
float ImageProcessor::getExposure() const { return exposure; }

// Creates OpenGL texture and FBOs for the RAW data and sets up the raw processing shader uniforms based on the image metadata.
// Also marks all render passes as dirty so the new image gets processed.
void ImageProcessor::uploadImage(std::shared_ptr<Image> image)
{
    currentImage = std::move(image);

    textureWidth = currentImage->getRawWidth();
    textureHeight = currentImage->getRawHeight();

    if (!rawTexture)
        rawTexture = new QOpenGLTexture(QOpenGLTexture::Target2D);

    // Set up new OpenGL texture.
    rawTexture->destroy();
    rawTexture->create();
    rawTexture->setSize(textureWidth, textureHeight);
    rawTexture->setFormat(QOpenGLTexture::R16_UNorm);
    rawTexture->setMinMagFilters(QOpenGLTexture::Nearest, QOpenGLTexture::Nearest);
    rawTexture->setWrapMode(QOpenGLTexture::ClampToEdge);
    rawTexture->allocateStorage(QOpenGLTexture::Red, QOpenGLTexture::UInt16);
    rawTexture->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt16, currentImage->getRawData());

    // Make sure FBOs are created and have the correct size for the new image.
    createFbos(textureWidth, textureHeight);

    // Set up shader uniforms for raw processing based on the new image metadata.
    rawToRGBProgram.bind();
    rawToRGBProgram.setUniformValue("wbMultipliers", currentImage->getWbMultipliers());
    rawToRGBProgram.setUniformValue("blackLevels", currentImage->getBlackLevels());
    rawToRGBProgram.setUniformValue("camToSRGB", currentImage->getCamToSrgb());
    rawToRGBProgram.setUniformValue("camToXYZ", currentImage->getCamToXyz());
    rawToRGBProgram.release();

    rawBitDirty = true;
    adjustmentBitDirty = true;
}

// Ensures that the FBOs are created and have the correct size. If they already exist with the correct size, does nothing.
void ImageProcessor::createFbos(int width, int height)
{
    if (fboRawToRGB && fboAdjustment && fboRawToRGB->size() == QSize(width, height) && fboAdjustment->size() == QSize(width, height))
        return;

    delete fboRawToRGB;
    delete fboAdjustment;

    QOpenGLFramebufferObjectFormat fboFormat;
    fboFormat.setAttachment(QOpenGLFramebufferObject::NoAttachment);
    fboFormat.setTextureTarget(GL_TEXTURE_2D);
    fboFormat.setInternalTextureFormat(GL_RGB16F);

    fboRawToRGB = new QOpenGLFramebufferObject(width, height, fboFormat);
    glBindTexture(GL_TEXTURE_2D, fboRawToRGB->texture());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    fboAdjustment = new QOpenGLFramebufferObject(width, height, fboFormat);
    glBindTexture(GL_TEXTURE_2D, fboAdjustment->texture());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
}

// Converts the RAW texture to RGB (performs demosaicing, white balance, color space conversion) and writes the result to fboRawToRGB.
void ImageProcessor::renderRawToRgbPass()
{
    fboRawToRGB->bind();
    glViewport(0, 0, textureWidth, textureHeight);
    glClear(GL_COLOR_BUFFER_BIT);

    rawToRGBProgram.bind();
    vaoRawToRGB.bind();
    rawTexture->bind(0);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    rawTexture->release();
    vaoRawToRGB.release();
    rawToRGBProgram.release();
    fboRawToRGB->release();
}

// Applies adjustments to the RGB texture and writes the result to fboAdjustment.
void ImageProcessor::renderAdjustmentPass()
{
    fboAdjustment->bind();
    glViewport(0, 0, textureWidth, textureHeight);
    glClear(GL_COLOR_BUFFER_BIT);

    adjustmentProgram.bind();
    vaoAdjustment.bind();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fboRawToRGB->texture());

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindTexture(GL_TEXTURE_2D, 0);
    vaoAdjustment.release();
    adjustmentProgram.release();
    fboAdjustment->release();
}
