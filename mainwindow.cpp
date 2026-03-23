#include "mainwindow.h"
#include "utility.h"
#include "stb_image.h"
#include "libraw/libraw.h"

#include <iostream>
#include <limits>
#include <cmath>
#include <vector>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setFixedSize(1080, 720);

    canvas = new Canvas(this);
    canvas->setGeometry(400, 50, 650, 600);

    btn = new QPushButton("Button!!!", this);
    btn->setGeometry(100, 100, 200, 50);
    btn->setToolTip("HAHAhahahaha something");

    slider = new QSlider(Qt::Horizontal, this);
    slider->setGeometry(100, 200, 200, 50);
    slider->setRange(0, 100);
    slider->setValue(50);

    pbar = new QProgressBar(this);
    pbar->setGeometry(100, 300, 200, 50);

    rBtn1 = new QRadioButton("1", this);
    rBtn1->setGeometry(100, 400, 50, 50);
    rBtn2 = new QRadioButton("2", this);
    rBtn2->setGeometry(150, 400, 50, 50);
    rBtn3 = new QRadioButton("3", this);
    rBtn3->setGeometry(200, 400, 50, 50);
    btnGroup = new QButtonGroup(this);
    btnGroup->addButton(rBtn1);
    btnGroup->addButton(rBtn2);
    btnGroup->addButton(rBtn3);

    /*pieMenu = new PieMenu(this);
    pieMenu->setButtonCount(5);*/

    QObject::connect(btn, &QPushButton::clicked, this, &MainWindow::onButtonClicked);
    //QObject::connect(btn, &QPushButton::pressed, pieMenu, &PieMenu::display);
    QObject::connect(slider, &QSlider::valueChanged, pbar, &QProgressBar::setValue);
    QObject::connect(slider, &QSlider::valueChanged, canvas, &Canvas::onSliderChanged);
}

MainWindow::~MainWindow() {}

void MainWindow::onButtonClicked()
{
    btn->setText("Clicked!!!");
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
        std::cout << "Escape key pressed" << std::endl;

    if (event->key() == Qt::Key_0)
        canvas->setCompareMode(0);
    else if (event->key() == Qt::Key_1)
        canvas->setCompareMode(1);
    else if (event->key() == Qt::Key_2)
        canvas->setCompareMode(2);
    else if (event->key() == Qt::Key_3)
        canvas->setCompareMode(3);
}

Canvas::Canvas(QWidget *parent)
    : QOpenGLWidget(parent), rawToRGBProgram(), adjustmentProgram(), displayProgram(),
      vbo(), vaoRawToRGB(), vaoAdjustment(), vaoDisplay(), fboRawToRGB(nullptr), fboAdjustment(nullptr),
        texture(nullptr), referenceTexture(nullptr), textureWidth(0), textureHeight(0), compareWithLibRaw(false), compareMode(0),
      isMouseDragging(false), mouseLastPos(0, 0), imgOffset(0, 0), imgZoom(0) {}

Canvas::~Canvas()
{
    // Destroy opengl resources
    makeCurrent();

    vbo.destroy();
    vaoRawToRGB.destroy();
    vaoAdjustment.destroy();
    vaoDisplay.destroy();
    delete texture;
    delete referenceTexture;
    delete fboRawToRGB;
    delete fboAdjustment;

    doneCurrent();
}

void Canvas::initializeGL()
{
    qDebug() << "----------------- START -----------------";

    // Set up OpenGL state
    initializeOpenGLFunctions();
    glDisable(GL_FRAMEBUFFER_SRGB);  // sRGB framebuffer for correct gamma handling (applies gamma correction when writing to framebuffer)
    glClearColor(.5f, .5f, .5f, 1.f);

    // Load, compile and link shaders
    std::string vertexShaderSourceRawToRGB = shaderFileToString("./shaders/rawtorgb.vert");
    std::string fragmentShaderSourceRawToRGB = shaderFileToString("./shaders/rawtorgb.frag");

    rawToRGBProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSourceRawToRGB.c_str());
    rawToRGBProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSourceRawToRGB.c_str());
    rawToRGBProgram.link();

    std::string vertexShaderSourceAdjustment = shaderFileToString("./shaders/adjustment.vert");
    std::string fragmentShaderSourceAdjustment = shaderFileToString("./shaders/adjustment.frag");

    adjustmentProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSourceAdjustment.c_str());
    adjustmentProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSourceAdjustment.c_str());
    adjustmentProgram.link();


    std::string vertexShaderSourceDisplay = shaderFileToString("./shaders/display.vert");
    std::string fragmentShaderSourceDisplay = shaderFileToString("./shaders/display.frag");

    displayProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSourceDisplay.c_str());
    displayProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSourceDisplay.c_str());
    displayProgram.link();

    /*posAttr = program->attributeLocation("pos");
    Q_ASSERT(m_posAttr != -1);
    colAttr = m_program->attributeLocation("col");
    Q_ASSERT(m_colAttr != -1);
    matrixUniform = m_program->uniformLocation("matrix");
    Q_ASSERT(m_matrixUniform != -1);*/

    float verts[] = {
        // X, Y,      U, V
        -1.f, -1.f,   0.f, 0.f,
         1.f, -1.f,   1.f, 0.f,
        -1.f,  1.f,   0.f, 1.f,
         1.f,  1.f,   1.f, 1.f
    };

    // Load RAW image using LibRaw
    LibRaw rawProcessor;
    int ret = rawProcessor.open_file("./textures/raw-img1.ARW");
    if (ret != LIBRAW_SUCCESS) {
        std::cerr << "Failed to open RAW file: " << LibRaw::strerror(ret) << std::endl;
        return;
    }

    ret = rawProcessor.unpack();    // extract and decode raw image data
    if (ret != LIBRAW_SUCCESS) {
        std::cerr << "Failed to unpack RAW file: " << LibRaw::strerror(ret) << std::endl;
        return;
    }

    // Build a LibRaw CPU-processed reference image for side-by-side comparison
    int referenceWidth = 0;
    int referenceHeight = 0;
    std::vector<ushort> referenceImage;
    if (compareWithLibRaw) {
        LibRaw referenceProcessor;
        //referenceProcessor.imgdata.params.output_color = 0;      // raw/camera RGB output (no output colorspace transform)
        //referenceProcessor.imgdata.params.use_camera_matrix = 0; // do not force camera->sRGB style matrix path
        ret = referenceProcessor.open_file("./textures/raw-img1.ARW");
        if (ret == LIBRAW_SUCCESS)
            ret = referenceProcessor.unpack();

        if (ret == LIBRAW_SUCCESS) {
            // Match GPU pipeline as closely as LibRaw allows.
            //referenceProcessor.imgdata.params.user_qual = 0;        // bilinear demosaic (lin_interpolate)
            //referenceProcessor.imgdata.params.four_color_rgb = 1;   // enable G1/G2 mixing path so interpolation runs as 3-color
            //referenceProcessor.imgdata.params.use_camera_wb = 0;    // force explicit user WB for deterministic compare
            //referenceProcessor.imgdata.params.use_auto_wb = 0;
            //referenceProcessor.imgdata.params.no_auto_scale = 0;    // keep scale_colors (black/white normalization + WB)
            //referenceProcessor.imgdata.params.no_interpolation = 0;
            //referenceProcessor.imgdata.params.highlight = 0;
            //referenceProcessor.imgdata.params.user_flip = 0;        // keep sensor/native orientation for direct compare

            // Force same WB source as GPU path: cam_mul values from metadata.
            for (int c = 0; c < 4; ++c)
                referenceProcessor.imgdata.params.user_mul[c] = referenceProcessor.imgdata.color.cam_mul[c];

            // Disable tone shaping so reference stays linear.
            /*
            referenceProcessor.imgdata.params.no_auto_bright = 1;
            referenceProcessor.imgdata.params.bright = 1.0f;
            referenceProcessor.imgdata.params.gamm[0] = 1.0;
            referenceProcessor.imgdata.params.gamm[1] = 0.0;
            referenceProcessor.imgdata.params.gamm[2] = 0.0;
            referenceProcessor.imgdata.params.gamm[3] = 0.0;
            referenceProcessor.imgdata.params.gamm[4] = 0.0;
            referenceProcessor.imgdata.params.gamm[5] = 0.0;
            referenceProcessor.imgdata.params.output_bps = 16;
            referenceProcessor.imgdata.params.exp_correc = 0;*/
            ret = referenceProcessor.dcraw_process();
        }
        
        if (ret == LIBRAW_SUCCESS && referenceProcessor.imgdata.image) {
            referenceWidth = referenceProcessor.imgdata.sizes.width;
            referenceHeight = referenceProcessor.imgdata.sizes.height;
            referenceImage.resize(referenceWidth * referenceHeight * 3);

            ushort (*img)[4] = referenceProcessor.imgdata.image;
            for (int y = 0; y < referenceHeight; ++y) {
                for (int x = 0; x < referenceWidth; ++x) {
                    const int srcIdx = y * referenceWidth + x;
                    const int dstIdx = (y * referenceWidth + x) * 3;
                    referenceImage[dstIdx + 0] = img[srcIdx][0];
                    referenceImage[dstIdx + 1] = img[srcIdx][1];
                    referenceImage[dstIdx + 2] = img[srcIdx][2];
                }
            }
        }

        // Reference image texture
        referenceTexture = new QOpenGLTexture(QOpenGLTexture::Target2D);
        referenceTexture->setSize(referenceWidth, referenceHeight);
        referenceTexture->setMinMagFilters(QOpenGLTexture::Nearest, QOpenGLTexture::Nearest);
        referenceTexture->setWrapMode(QOpenGLTexture::ClampToEdge);

        referenceTexture->setFormat(QOpenGLTexture::RGB16_UNorm);
        referenceTexture->allocateStorage(QOpenGLTexture::RGB, QOpenGLTexture::UInt16);
        referenceTexture->setData(QOpenGLTexture::RGB, QOpenGLTexture::UInt16, referenceImage.data());
    }
    
    // Retrieve RAW image data
    ushort* rawData = rawProcessor.imgdata.rawdata.raw_image;
    const int rawWidth = rawProcessor.imgdata.sizes.raw_width;
    const int rawHeight = rawProcessor.imgdata.sizes.raw_height;
    const int imageWidth = rawProcessor.imgdata.sizes.width;
    const int imageHeight = rawProcessor.imgdata.sizes.height;
    const int leftMargin = rawProcessor.imgdata.sizes.left_margin;
    const int topMargin = rawProcessor.imgdata.sizes.top_margin;
    // textureWidth = imageWidth;
    // textureHeight = imageHeight;
    textureWidth = rawWidth;
    textureHeight = rawHeight;

    // Crop raw sensor buffer to active image area so GPU and LibRaw reference use the same dimensions
    /*
    std::vector<ushort> rawActive(textureWidth * textureHeight);
    for (int y = 0; y < textureHeight; ++y) {
        const int srcY = y + topMargin;
        const ushort* src = rawData + srcY * rawWidth + leftMargin;
        ushort* dst = rawActive.data() + y * textureWidth;
        std::copy(src, src + textureWidth, dst);
    }*/

    const uint* cblack = rawProcessor.imgdata.color.cblack;    // per-channel black offsets
    const uint black = rawProcessor.imgdata.color.black;       // global black level offset
    const uint white = rawProcessor.imgdata.color.maximum;     // white level (maximum sensor value)

    QVector4D blackLevels = QVector4D(
        float(cblack[0] + black),
        float(cblack[1] + black),
        float(cblack[2] + black),
        float(cblack[3] + black)
    );

    // Retrieve and normalize WB coefficients
    const float maxMinusBlack = std::max(1.0f, float(white) - float(black));
    float wbMul[4] = {
        rawProcessor.imgdata.color.cam_mul[0],
        rawProcessor.imgdata.color.cam_mul[1],
        rawProcessor.imgdata.color.cam_mul[2],
        rawProcessor.imgdata.color.cam_mul[3]
    };

    const float greenMul = wbMul[1];    // normalize WB coefficients by the green channel value
    for (int c = 0; c < 4; ++c)
        wbMul[c] = (wbMul[c] / greenMul) / maxMinusBlack;
    
    // Prepare color space conversion matrices
    QMatrix3x3 camToSRGBMatrix;  // merge LibRaw's two green channels (G1+G2) into a 3x3 RGB matrix
    for (int i = 0; i < 3; i++) {
        camToSRGBMatrix(i, 0) = rawProcessor.imgdata.color.rgb_cam[i][0];                               // R column
        camToSRGBMatrix(i, 1) = rawProcessor.imgdata.color.rgb_cam[i][1] + rawProcessor.imgdata.color.rgb_cam[i][3]; // G column (G1 + G2)
        camToSRGBMatrix(i, 2) = rawProcessor.imgdata.color.rgb_cam[i][2];                               // B column
    }

    QMatrix3x3 camToXYZMatrix;  // retrieve XYZ to camera RGB color space conversion matrix
    for(int i = 0; i < 3; i++) {
        camToXYZMatrix(i, 0) = rawProcessor.imgdata.color.cam_xyz[i][0];
        camToXYZMatrix(i, 1) = rawProcessor.imgdata.color.cam_xyz[i][1];
        camToXYZMatrix(i, 2) = rawProcessor.imgdata.color.cam_xyz[i][2];
    }
    
    // float XYZtoSRGB[9] = {   // standard XYZ to sRGB matrix
    //     3.2406f, -1.5372f, -0.4986f,
    //    -0.9689f,  1.8758f,  0.0415f,
    //     0.0557f, -0.2040f,  1.0570f
    // };
    // QMatrix3x3 XYZtoSRGBMatrix = QMatrix3x3(XYZtoSRGB);

    // Print RAW image debug info
    qDebug() << "-------------- RAW IMAGE INFO --------------";
    qDebug() << "CFA pattern: " << rawProcessor.imgdata.idata.cdesc;
    qDebug() << "Raw image dimensions: " << rawWidth << ", " << rawHeight;
    qDebug() << "Real image dimensions: " << imageWidth << ", " << imageHeight;
    qDebug() << "CFA phase offset (left, top):" << leftMargin << "," << topMargin;
    qDebug() << "cblack levels:" << cblack[0] << ", " << cblack[1] << ", " << cblack[2] << ", " << cblack[3];
    qDebug() << "global black level:" << black;
    qDebug() << "white level:" << white;
    qDebug() << "WB multipliers (R,G1,B,G2):"
             << wbMul[0] << "," << wbMul[1] << ","
             << wbMul[2] << "," << wbMul[3];

    // Create OpenGL texture for the raw image data
    texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    texture->setSize(textureWidth, textureHeight);
    texture->setFormat(QOpenGLTexture::R16_UNorm);
    texture->setMinMagFilters(QOpenGLTexture::Nearest, QOpenGLTexture::Nearest);
    texture->setWrapMode(QOpenGLTexture::ClampToEdge);

    texture->allocateStorage(QOpenGLTexture::Red, QOpenGLTexture::UInt16);
    texture->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt16, rawData);


    // Load and set the texture
    /*
    stbi_set_flip_vertically_on_load(true);
    int nrChannels;
    unsigned char *data = stbi_load("./textures/simono-totalne-na-dne.jpg", &textureWidth, &textureHeight, &nrChannels, 0);
    
    texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    texture->setSize(textureWidth, textureHeight);
    texture->setFormat(QOpenGLTexture::RGB8_UNorm);
    //texture->setFormat(QOpenGLTexture::RGB16F);
    texture->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
    texture->setWrapMode(QOpenGLTexture::ClampToEdge);
    texture->allocateStorage(QOpenGLTexture::RGB, QOpenGLTexture::UInt8);
    //texture->allocateStorage(QOpenGLTexture::RGB, QOpenGLTexture::Float32);
    texture->setData(QOpenGLTexture::RGB, QOpenGLTexture::UInt8, data);
    //texture->setData(QOpenGLTexture::RGB, QOpenGLTexture::Float32, data);

    stbi_image_free(data);*/

    // Create FBOs and attach textures
    QOpenGLFramebufferObjectFormat fboFormat;
    fboFormat.setAttachment(QOpenGLFramebufferObject::NoAttachment);
    fboFormat.setTextureTarget(GL_TEXTURE_2D);
    fboFormat.setInternalTextureFormat(GL_RGB16F);

    fboRawToRGB = new QOpenGLFramebufferObject(textureWidth, textureHeight, fboFormat);

    glBindTexture(GL_TEXTURE_2D, fboRawToRGB->texture());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    fboAdjustment = new QOpenGLFramebufferObject(textureWidth, textureHeight, fboFormat);

    glBindTexture(GL_TEXTURE_2D, fboAdjustment->texture());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Create VBO
    vbo.create();
    vbo.bind();
    vbo.allocate(verts, sizeof(verts));

    // Set vertex attributes for process pass
    vaoRawToRGB.create();
    vaoRawToRGB.bind();

    rawToRGBProgram.bind();
    rawToRGBProgram.enableAttributeArray(0);
    rawToRGBProgram.setAttributeBuffer(0, GL_FLOAT, 0, 2, 4 * sizeof(GLfloat));
    rawToRGBProgram.enableAttributeArray(1);
    rawToRGBProgram.setAttributeBuffer(1, GL_FLOAT, 2 * sizeof(GLfloat), 2, 4 * sizeof(GLfloat));
    rawToRGBProgram.setUniformValue("imageTex", 0);
    rawToRGBProgram.setUniformValue("wbMultipliers", QVector4D(wbMul[0], wbMul[1], wbMul[2], wbMul[3]));
    rawToRGBProgram.setUniformValue("cfaOffset", 0, 0);
    rawToRGBProgram.setUniformValue("blackLevels", blackLevels);
    rawToRGBProgram.setUniformValue("camToSRGB", camToSRGBMatrix);
    rawToRGBProgram.setUniformValue("camToXYZ", camToXYZMatrix);
    rawToRGBProgram.release();

    vaoRawToRGB.release();

    // Set vertex attributes for adjustment pass
    vaoAdjustment.create();
    vaoAdjustment.bind();

    adjustmentProgram.bind();
    adjustmentProgram.enableAttributeArray(0);
    adjustmentProgram.setAttributeBuffer(0, GL_FLOAT, 0, 2, 4 * sizeof(GLfloat));
    adjustmentProgram.enableAttributeArray(1);
    adjustmentProgram.setAttributeBuffer(1, GL_FLOAT, 2 * sizeof(GLfloat), 2, 4 * sizeof(GLfloat));
    adjustmentProgram.setUniformValue("imageTex", 0);
    adjustmentProgram.setUniformValue("exposure", 0.5f);
    adjustmentProgram.release();

    vaoAdjustment.release();

    // Set vertex attributes for display pass
    vaoDisplay.create();
    vaoDisplay.bind();

    displayProgram.bind();
    displayProgram.enableAttributeArray(0);
    displayProgram.setAttributeBuffer(0, GL_FLOAT, 0, 2, 4 * sizeof(GLfloat));
    displayProgram.enableAttributeArray(1);
    displayProgram.setAttributeBuffer(1, GL_FLOAT, 2 * sizeof(GLfloat), 2, 4 * sizeof(GLfloat));
    displayProgram.setUniformValue("imageTex", 0);
    displayProgram.setUniformValue("referenceTex", 1);
    displayProgram.setUniformValue("compareMode", compareWithLibRaw ? compareMode : 0);
    QMatrix4x4 transformMatrix = QMatrix4x4();
    displayProgram.setUniformValue("transform", transformMatrix);
    updateTransform();

    displayProgram.release();

    vaoDisplay.release();
    
    vbo.release();

    qDebug() << "----------------- END -----------------";
}

void Canvas::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

void Canvas::paintGL()
{
    if (!texture || !fboRawToRGB || textureWidth <= 0 || textureHeight <= 0)
        return;

    // pass 1: Raw to RGB conversion pass
    fboRawToRGB->bind();
    glViewport(0, 0, textureWidth, textureHeight);  // resize viewport to the image resolution
    glClear(GL_COLOR_BUFFER_BIT);

    rawToRGBProgram.bind();
    vaoRawToRGB.bind();
    texture->bind();

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    texture->release();
    vaoRawToRGB.release();
    rawToRGBProgram.release();

    // pass 2: Adjustment pass
    fboAdjustment->bind();
    //glViewport(0, 0, textureWidth, textureHeight);  // resize viewport to the image resolution
    glClear(GL_COLOR_BUFFER_BIT);

    adjustmentProgram.bind();
    vaoAdjustment.bind();

    GLuint textureId = fboRawToRGB->texture();  // bind the texture from the previous pass
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    vaoAdjustment.release();
    adjustmentProgram.release();

    // pass 3: Display pass
    fboAdjustment->release();                           // back to default framebuffer
    float dpr = devicePixelRatioF();                    // high-DPI display scaling
    glViewport(0, 0, width() * dpr, height() * dpr);    // reset viewport to widget size
    glClear(GL_COLOR_BUFFER_BIT);

    displayProgram.bind();
    vaoDisplay.bind();

    textureId = fboAdjustment->texture();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);

    glActiveTexture(GL_TEXTURE1);   // libraw reference image
    if (compareWithLibRaw && referenceTexture)
        referenceTexture->bind();
    else
        glBindTexture(GL_TEXTURE_2D, textureId);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    vaoDisplay.release();
    displayProgram.release();
}

void Canvas::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        isMouseDragging = true;
        mouseLastPos = event->pos();
    }
}

void Canvas::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        isMouseDragging = false;
}

void Canvas::mouseMoveEvent(QMouseEvent *event)
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

void Canvas::wheelEvent(QWheelEvent *event)
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
    imgZoom = std::clamp(imgZoom, 0.01f, 20.f);

    updateTransform();
}

void Canvas::updateTransform()
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

    // Preserve image aspect ratio
    float textureAspect = float(textureWidth) / float(textureHeight);
    if (textureAspect > 1.f) {  // texture width > height
        float scaleY = 1.f / textureAspect;
        transformMatrix.scale(1.f, scaleY, 1.f);
    } else {                    // texture height > width
        float scaleX = textureAspect;
        transformMatrix.scale(scaleX, 1.f, 1.f);
    }

    displayProgram.bind();
    displayProgram.setUniformValue("transform", transformMatrix);
    displayProgram.release();

    update();   // trigger repaint
}

void Canvas::onSliderChanged(int value)
{
    float exposure = (float)value / 100.f;

    adjustmentProgram.bind();
    adjustmentProgram.setUniformValue("exposure", exposure);
    adjustmentProgram.release();

    update();   // trigger repaint
}

void Canvas::setCompareMode(int mode)
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
