#include "imageviewer.h"
#include "../utility.h"

#include <QMouseEvent>

ImageViewer::ImageViewer(QWidget *parent)
        : QOpenGLWidget(parent), displayProgram(), vaoDisplay(), image(nullptr), imageProcessor(),
            referenceTexture(nullptr), textureWidth(0), textureHeight(0), compareWithLibRaw(false), compareMode(0),
      isMouseDragging(false), mouseLastPos(0, 0), imgOffset(0, 0), imgZoom(0) {}

ImageViewer::~ImageViewer()
{
    // Destroy opengl resources
    makeCurrent();

    imageProcessor.cleanupGL();
    vaoDisplay.destroy();
    delete referenceTexture;
    referenceTexture = nullptr;

    doneCurrent();
}

void ImageViewer::initializeGL()
{
    // Set up OpenGL state
    initializeOpenGLFunctions();
    //glEnable(GL_FRAMEBUFFER_SRGB);  // sRGB framebuffer for correct gamma handling (applies gamma correction when writing to framebuffer)
    glClearColor(.5f, .5f, .5f, 1.f);

    // Load, compile and link shaders
    std::string vertexShaderSourceDisplay = shaderFileToString("./shaders/display.vert");
    std::string fragmentShaderSourceDisplay = shaderFileToString("./shaders/display.frag");

    displayProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSourceDisplay.c_str());
    displayProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSourceDisplay.c_str());
    displayProgram.link();

    // Initialize image processor (compile shaders, create FBOs, etc).
    imageProcessor.initializeGL();

    // Set vertex attributes and uniforms for display pass
    const float verts[] = {
        -1.f, -1.f, 0.f, 0.f,
         1.f, -1.f, 1.f, 0.f,
        -1.f,  1.f, 0.f, 1.f,
         1.f,  1.f, 1.f, 1.f
    };

    vbo.create();
    vbo.bind();
    vbo.allocate(verts, sizeof(verts));

    vaoDisplay.create();
    vaoDisplay.bind();

    displayProgram.bind();
    displayProgram.enableAttributeArray(0);
    displayProgram.setAttributeBuffer(0, GL_FLOAT, 0, 2, 4 * sizeof(GLfloat));
    displayProgram.enableAttributeArray(1);
    displayProgram.setAttributeBuffer(1, GL_FLOAT, 2 * sizeof(GLfloat), 2, 4 * sizeof(GLfloat));
    displayProgram.release();

    vaoDisplay.release();
    vbo.release();

    displayProgram.bind();
    displayProgram.setUniformValue("imageTex", 0);
    displayProgram.setUniformValue("referenceTex", 1);
    displayProgram.setUniformValue("compareMode", compareWithLibRaw ? compareMode : 0);
    QMatrix4x4 transformMatrix = QMatrix4x4();
    displayProgram.setUniformValue("transform", transformMatrix);
    displayProgram.release();

    updateTransform();
}

void ImageViewer::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    updateTransform();
}

void ImageViewer::paintGL()
{
    imageProcessor.processImage(image);
    const GLuint processedTexture = imageProcessor.getProcessedTexture();
    if (!processedTexture || textureWidth <= 0 || textureHeight <= 0)
        return;

    // Display pass
    float dpr = devicePixelRatioF();                    // high-DPI display scaling
    glViewport(0, 0, width() * dpr, height() * dpr);    // reset viewport to widget size
    glClear(GL_COLOR_BUFFER_BIT);

    displayProgram.bind();
    vaoDisplay.bind();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, processedTexture);

    glActiveTexture(GL_TEXTURE1);   // libraw reference image
    if (compareWithLibRaw && referenceTexture)
        referenceTexture->bind();
    else
        glBindTexture(GL_TEXTURE_2D, processedTexture);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    vaoDisplay.release();
    displayProgram.release();
}

void ImageViewer::setImage(std::shared_ptr<Image> newImage)
{
    // Load RAW image and metadata
    image = newImage;
    bool res = image->loadRawData();
    if (!res) {
        qWarning() << "Failed to load RAW file.";
        return;
    }

    // Load adjustment cells for the image or generate default ones if not present
    image->loadAdjustmentCells();

    textureWidth = image->getRawWidth();
    textureHeight = image->getRawHeight();

    // Create OpenGL texture for libraw-processed reference image if comparison mode enabled
    if (compareWithLibRaw && image->buildReferenceImage()) {
        referenceTexture = new QOpenGLTexture(QOpenGLTexture::Target2D);
        referenceTexture->setSize(image->getReferenceWidth(), image->getReferenceHeight());
        referenceTexture->setMinMagFilters(QOpenGLTexture::Nearest, QOpenGLTexture::Nearest);
        referenceTexture->setWrapMode(QOpenGLTexture::ClampToEdge);
        referenceTexture->setFormat(QOpenGLTexture::RGB16_UNorm);
        referenceTexture->allocateStorage(QOpenGLTexture::RGB, QOpenGLTexture::UInt16);
        referenceTexture->setData(QOpenGLTexture::RGB, QOpenGLTexture::UInt16, image->getReferenceData());
    }

    updateTransform();
}

// --- Input event handlers ---

void ImageViewer::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        isMouseDragging = true;
        mouseLastPos = event->pos();
    }
}

void ImageViewer::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        isMouseDragging = false;
}

void ImageViewer::mouseMoveEvent(QMouseEvent *event)
{
    if (isMouseDragging) {
        QPointF delta = event->pos() - mouseLastPos;
        mouseLastPos = event->pos();

        delta.rx() *= -1;  // invert y axis
        float scale = std::pow(2.f, imgZoom);   // adjust for zoom level
        delta /= scale;

        imgOffset -= delta;

        updateTransform();
    }
}

void ImageViewer::wheelEvent(QWheelEvent *event)
{
    QPoint numPixels = event->pixelDelta(); // .y = vertical scroll wheel
    QPoint numDegrees = event->angleDelta() / 8;

    if (!numPixels.isNull()) {
        imgZoom += numPixels.y() * 0.1f;
    } else if (!numDegrees.isNull()) {
        QPoint numSteps = numDegrees / 15;
        imgZoom += numSteps.y() * 0.1f;
    }

    //qDebug() << "Image Zoom: " << imgZoom;
    //imgZoom = std::clamp(imgZoom, 0.01f, 20.f);

    updateTransform();
}

void ImageViewer::updateTransform()
{
    if (textureWidth <= 0 || textureHeight <= 0)
        return;

    QMatrix4x4 transformMatrix = QMatrix4x4();

    // Apply zoom
    float scale = std::pow(2.f, imgZoom);
    transformMatrix.scale(scale, scale, 1.f);

    // Apply pan
    float xOffset = (2 / float(width())) * imgOffset.x();  // convert from pixel offset to NDC
    float yOffset = (2 / float(height())) * imgOffset.y();
    transformMatrix.translate(xOffset, yOffset, 0.f);

    // Preserve image aspect ratio relative to the viewer widget dimensions
    const float textureAspect = float(textureWidth) / float(textureHeight);
    const float widgetAspect  = float(width()) / float(height());
    const float combinedAspect = textureAspect / widgetAspect;
    if (combinedAspect > 1.f) {  // texture width > height
        transformMatrix.scale(1.f, 1.f / combinedAspect, 1.f);
    } else {                     // texture height > width
        transformMatrix.scale(combinedAspect, 1.f, 1.f);
    }

    displayProgram.bind();
    displayProgram.setUniformValue("transform", transformMatrix);
    displayProgram.release();

    update();   // trigger repaint
}

void ImageViewer::onAdjustmentChanged()
{
    imageProcessor.markAdjustmentDirty();
    update();
}

void ImageViewer::setCompareMode(int mode)
{
    compareMode = std::clamp(mode, 0, 3);

    if (!context())
        return;

    makeCurrent();
    displayProgram.bind();
    displayProgram.setUniformValue("compareMode", compareWithLibRaw ? compareMode : 0);
    displayProgram.release();
    doneCurrent();

    update();
}
