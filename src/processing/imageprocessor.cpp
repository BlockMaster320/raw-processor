#include "imageprocessor.h"
#include "core/utility.h"

#include <QDebug>
#include <QImage>
#include <QOpenGLShader>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <utility>

ImageProcessor::ImageProcessor()
    : rawToRGBProgram(), adjustmentProgram(), rgbToYcbcrProgram(), gaussianChromaProgram(),
    bilateralLumaProgram(), ycbcrToRgbProgram(), postprocessProgram(), gammaProgram(), vbo(), vaoRawToRGB(), vaoAdjustment(),
      fboRgb(nullptr), fboAdjustment1(nullptr), fboAdjustment2(nullptr), fboFree(nullptr), fboOccupied(nullptr), isFboRgbUsed(false), rawTexture(nullptr),
      textureWidth(0), textureHeight(0), initialized(false),
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
    delete fboRgb;
    fboRgb = nullptr;
    delete fboAdjustment1;
    fboAdjustment1 = nullptr;
    delete fboAdjustment2;
    fboAdjustment2 = nullptr;

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

    const std::string vertexShaderSourceRawToRGB = shaderFileToString(":/shaders/rawtorgb.vert");
    const std::string fragmentShaderSourceRawToRGB = shaderFileToString(":/shaders/rawtorgb.frag");

    rawToRGBProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSourceRawToRGB.c_str());
    rawToRGBProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSourceRawToRGB.c_str());
    rawToRGBProgram.link();

    const std::string vertexShaderSourcePass = shaderFileToString(":/shaders/pass.vert");
    const std::string fragmentShaderSourceAdjustment = shaderFileToString(":/shaders/adjustment.frag");
    const std::string fragmentShaderSourceRgbToYcbcr = shaderFileToString(":/shaders/rgbtoycbcr.frag");
    const std::string fragmentShaderSourceGaussianChroma = shaderFileToString(":/shaders/gaussianchroma.frag");
    const std::string fragmentShaderSourceBilateralLuma = shaderFileToString(":/shaders/bilateralluma.frag");
    const std::string fragmentShaderSourceYcbcrToRgb = shaderFileToString(":/shaders/ycbcrtorgb.frag");
    const std::string fragmentShaderSourcePostProcess = shaderFileToString(":/shaders/postprocess.frag");
    const std::string fragmentShaderSourceGamma = shaderFileToString(":/shaders/gamma.frag");

    adjustmentProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSourcePass.c_str());
    adjustmentProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSourceAdjustment.c_str());
    adjustmentProgram.link();

    rgbToYcbcrProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSourcePass.c_str());
    rgbToYcbcrProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSourceRgbToYcbcr.c_str());
    rgbToYcbcrProgram.link();

    gaussianChromaProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSourcePass.c_str());
    gaussianChromaProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSourceGaussianChroma.c_str());
    gaussianChromaProgram.link();

    bilateralLumaProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSourcePass.c_str());
    bilateralLumaProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSourceBilateralLuma.c_str());
    bilateralLumaProgram.link();

    ycbcrToRgbProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSourcePass.c_str());
    ycbcrToRgbProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSourceYcbcrToRgb.c_str());
    ycbcrToRgbProgram.link();

    postprocessProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSourcePass.c_str());
    postprocessProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSourcePostProcess.c_str());
    postprocessProgram.link();

    gammaProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSourcePass.c_str());
    gammaProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSourceGamma.c_str());
    gammaProgram.link();

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

    rgbToYcbcrProgram.bind();
    rgbToYcbcrProgram.setUniformValue("imageTex", 0);
    rgbToYcbcrProgram.release();

    gaussianChromaProgram.bind();
    gaussianChromaProgram.setUniformValue("imageTex", 0);
    gaussianChromaProgram.release();

    bilateralLumaProgram.bind();
    bilateralLumaProgram.setUniformValue("imageTex", 0);
    bilateralLumaProgram.release();

    ycbcrToRgbProgram.bind();
    ycbcrToRgbProgram.setUniformValue("imageTex", 0);
    ycbcrToRgbProgram.release();

    postprocessProgram.bind();
    postprocessProgram.setUniformValue("imageTex", 0);
    postprocessProgram.release();

    gammaProgram.bind();
    gammaProgram.setUniformValue("imageTex", 0);
    gammaProgram.release();

    vaoAdjustment.release();
    vbo.release();

    initialized = true;
}

ImageProcessor::FilterAdjUniforms& ImageProcessor::FilterAdjUniforms::setFloat(const std::string& name, float value)
{
    floatUniforms[name] = value;
    return *this;
}

ImageProcessor::FilterAdjUniforms& ImageProcessor::FilterAdjUniforms::setInt(const std::string& name, int value)
{
    intUniforms[name] = value;
    return *this;
}

// Main processing function. Uploads the loaded image to GPU (if it's a new image) and runs the shader passes to produce the final processed texture.
// If the image is unchanged and the passes are not marked dirty, does nothing.
void ImageProcessor::processImage(std::shared_ptr<Image> image, bool gammaCorrect)
{
    if (!initialized || !image || !image->getIsLoaded())
        return;

    bool rerendered = false;

    if (currentImage.get() != image.get()) // new image, upload and mark passes dirty; this check might be ineffective, refactor later
        uploadImage(image);

    if (!rawTexture || !fboRgb || !fboAdjustment2 || textureWidth <= 0 || textureHeight <= 0)
        return;

    if (rawBitDirty) {
        renderRawToRgbPass();
        rawBitDirty = false;
        adjustmentBitDirty = true;
        rerendered = true;
    }

    if (adjustmentBitDirty) {
        renderAdjustmentPass();
        adjustmentBitDirty = false;
        rerendered = true;
    }

    if (gammaCorrect) {
        renderGammaPass();  // gamma correction
    }
}

// Processes the given image for export and fills the provided buffer with the image data. Returns false on failure (e.g. no image loaded).
bool ImageProcessor::processForExport(std::shared_ptr<Image> image, std::vector<unsigned char>& outBuffer, int& outWidth, int& outHeight)
{
    outBuffer.clear();
    outWidth = 0;
    outHeight = 0;

    if (!initialized || !image)
        return false;

    if (!image->getIsLoaded() && !image->loadRawData())
        return false;

    processImage(image, true);

    if (!fboOccupied) return false;

    QImage exportImage = fboOccupied->toImage();
    if (exportImage.isNull()) return false;

    if (exportImage.format() != QImage::Format_RGB888)
        exportImage = exportImage.convertToFormat(QImage::Format_RGB888);

    outWidth = exportImage.width();
    outHeight = exportImage.height();
    outBuffer.resize(static_cast<size_t>(outWidth) * static_cast<size_t>(outHeight) * 3u);

    const int rowBytes = outWidth * 3;
    for (int y = 0; y < outHeight; ++y) {   // copy each scanned line from the QImage to the output buffer
        const unsigned char* src = exportImage.constScanLine(y);
        std::memcpy(outBuffer.data() + (static_cast<size_t>(y) * rowBytes), src, static_cast<size_t>(rowBytes));
    }

    return true;
}

GLuint ImageProcessor::getProcessedTexture() const { return fboOccupied ? fboOccupied->texture() : 0; }
int ImageProcessor::getProcessedWidth() const { return textureWidth; }
int ImageProcessor::getProcessedHeight() const { return textureHeight; }

void ImageProcessor::markAdjustmentDirty() { adjustmentBitDirty = true; }

void ImageProcessor::setUniforms()
{
    adjustmentProgram.setUniformValue("temperature", uniforms.temperature);
    adjustmentProgram.setUniformValue("tint", uniforms.tint);
    adjustmentProgram.setUniformValue("exposure", uniforms.exposure);
    adjustmentProgram.setUniformValue("contrast", uniforms.contrast);
    adjustmentProgram.setUniformValue("midpoint", uniforms.midpoint);
    adjustmentProgram.setUniformValue("popArt", uniforms.popArt);
    adjustmentProgram.setUniformValue("white", uniforms.white);
    adjustmentProgram.setUniformValue("black", uniforms.black);
    adjustmentProgram.setUniformValue("saturation", uniforms.saturation);
    adjustmentProgram.setUniformValue("vignette", uniforms.vignette);

    // qDebug() << "Set uniforms: exposure=" << uniforms.exposure << " contrast=" << uniforms.contrast << " midpoint=" << uniforms.midpoint;
}
void ImageProcessor::resetUniforms() { uniforms = GlobalAdjUniforms{}; }

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
    setupFbos(textureWidth, textureHeight);

    // Set up shader uniforms for raw processing based on the new image metadata.
    rawToRGBProgram.bind();
    rawToRGBProgram.setUniformValue("wbMultipliers", currentImage->getWbMultipliers());
    rawToRGBProgram.setUniformValue("blackLevels", currentImage->getBlackLevels());
    rawToRGBProgram.setUniformValue("camToSRGB", currentImage->getCamToSrgb());
    rawToRGBProgram.setUniformValue("camToXYZ", currentImage->getCamToXyz());
    rawToRGBProgram.setUniformValue("camToRec2020", currentImage->getCamToRec2020());
    rawToRGBProgram.release();

    postprocessProgram.bind();
    postprocessProgram.setUniformValue("rec2020ToSrgb", currentImage->getRec2020ToSrgb());
    postprocessProgram.release();

    rawBitDirty = true;
    adjustmentBitDirty = true;
}

// Ensures that the FBOs are created and have the correct size. If they already exist with the correct size, does nothing.
void ImageProcessor::setupFbos(int width, int height)
{
    if (fboRgb && fboAdjustment1 && fboAdjustment2 && fboRgb->size() == QSize(width, height) && fboAdjustment1->size() == QSize(width, height) && fboAdjustment2->size() == QSize(width, height))
        return;

    delete fboRgb;
    delete fboAdjustment1;
    delete fboAdjustment2;

    QOpenGLFramebufferObjectFormat fboFormat;
    fboFormat.setAttachment(QOpenGLFramebufferObject::NoAttachment);
    fboFormat.setTextureTarget(GL_TEXTURE_2D);
    fboFormat.setInternalTextureFormat(GL_RGB16F);

    fboRgb = new QOpenGLFramebufferObject(width, height, fboFormat);
    glBindTexture(GL_TEXTURE_2D, fboRgb->texture());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    fboAdjustment1 = new QOpenGLFramebufferObject(width, height, fboFormat);
    glBindTexture(GL_TEXTURE_2D, fboAdjustment1->texture());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    fboAdjustment2 = new QOpenGLFramebufferObject(width, height, fboFormat);
    glBindTexture(GL_TEXTURE_2D, fboAdjustment2->texture());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
}

 // Implements frame buffer swapping (ping-pong) mechanism.
void ImageProcessor::swapFbos()
{
    QOpenGLFramebufferObject* temp = fboFree;
    fboFree = fboOccupied;
    fboOccupied = temp;

    if (!isFboRgbUsed) {
        fboFree = fboAdjustment2;
        isFboRgbUsed = true;
    }
}

// Returns FBO which is currently free to be written to (free: true) or contains texture to be read from (free: false).
// QOpenGLFramebufferObject* ImageProcessor::getFbo(bool free)
// {
//     if (fboSwap == true)
//         return (free) ? fboAdjustment2 : fboRgb;
//     else
//         return (free) ? fboRgb : fboAdjustment2;
// }

// --- RENDERING PASSES ---

// Converts the RAW texture to RGB (performs demosaicing, white balance, color space conversion) and writes the result to fboRawToRGB.
void ImageProcessor::renderRawToRgbPass()
{
    glDisable(GL_FRAMEBUFFER_SRGB); // keep offscreen processing linear; display gamma is handled in the final display pass

    fboRgb->bind();
    glViewport(0, 0, textureWidth, textureHeight);
    glClear(GL_COLOR_BUFFER_BIT);

    rawToRGBProgram.bind();
    vaoRawToRGB.bind();
    rawTexture->bind(0);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    rawTexture->release();
    vaoRawToRGB.release();
    rawToRGBProgram.release();
    fboRgb->release();
}

// Loops through the current image's AdjustmentCells, calls each Adjustment::apply(),
// then flushes any accumulated global uniforms via renderGlobalAdjustments().
void ImageProcessor::renderAdjustmentPass()
{
    if (!currentImage) return;

    glDisable(GL_FRAMEBUFFER_SRGB); // keep offscreen processing linear; display gamma is handled in the final display pass

    // Set up FBO pointers
    isFboRgbUsed = false;
    fboOccupied = fboRgb;
    fboFree = fboAdjustment1;

    for (auto& cell : currentImage->adjustmentCells) {
        // Check cell visibility (both instance visibility and data enabled state)
        if (!cell.isVisible || !cell.adjustmentGroup || !cell.adjustmentGroup->isEnabled)
            continue;

        // Process each cell as its own adjustment stage so non-additive uniforms
        // (e.g. midpoint) are not overwritten by later cells.
        resetUniforms();
        uniformsDirty = false;

        // Apply adjustments in process order
        for (AdjType type : cell.processOrder) {
            auto it = cell.adjustmentGroup->adjustments.find(type);
            if (it != cell.adjustmentGroup->adjustments.end())
                it->second->apply(*this);
        }

        renderGlobalAdjustments();  // flush any accumulated global adjustment uniforms before processing the next cell
    }

    renderPostprocessPass();    // tone mapping and Rec.2020 -> sRGB conversion
}


// Renders a generic fullscreen pass using the specified shader program and optional uniforms.
void ImageProcessor::renderGenericPass(QOpenGLShaderProgram& program, const FilterAdjUniforms* localUniforms)
{
    fboFree->bind();
    glViewport(0, 0, textureWidth, textureHeight);
    glClear(GL_COLOR_BUFFER_BIT);

    program.bind();
    vaoAdjustment.bind();

    if (localUniforms) {
        for (const auto& [name, value] : localUniforms->floatUniforms)
            program.setUniformValue(name.c_str(), value);
        for (const auto& [name, value] : localUniforms->intUniforms)
            program.setUniformValue(name.c_str(), value);
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fboOccupied->texture());

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindTexture(GL_TEXTURE_2D, 0);
    vaoAdjustment.release();
    program.release();
    fboOccupied->release();

    swapFbos();
}


// Uploads accumulated GlobalAdjUniforms to the adjustment shader and renders a fullscreen pass.
void ImageProcessor::renderGlobalAdjustments()
{
    if (!uniformsDirty) return;

    adjustmentProgram.bind();
    setUniforms();
    adjustmentProgram.release();

    renderGenericPass(adjustmentProgram);

    uniformsDirty = false;
    resetUniforms();
}

// Renders the final postprocess pass (tone mapping + Rec.2020 -> sRGB conversion).
void ImageProcessor::renderPostprocessPass()
{
    renderGenericPass(postprocessProgram);
}

// Renders gamma-correction as the final optional pass used for export output.
void ImageProcessor::renderGammaPass()
{
    renderGenericPass(gammaProgram);
}

// Renders a filter pass with optional uniforms.
void ImageProcessor::renderFilterPass(FilterPassType passType, const FilterAdjUniforms& uniforms)
{
    QOpenGLShaderProgram* program = nullptr;
    switch (passType) { // get the shader program for the requested filter pass type
        case FilterPassType::RgbToYcbcr:
            program = &rgbToYcbcrProgram;
            break;
        case FilterPassType::GaussianChroma:
            program = &gaussianChromaProgram;
            break;
        case FilterPassType::BilateralLuma:
            program = &bilateralLumaProgram;
            break;
        case FilterPassType::YcbcrToRgb:
            program = &ycbcrToRgbProgram;
            break;
    }

    if (!program)
        return;

    renderGenericPass(*program, &uniforms);
}
