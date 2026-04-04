#include "mainwindow.h"
#include "imagemanager.h"
#include "gallerywidget.h"
#include "utility.h"

#include <QHBoxLayout>
#include <QSplitter>

#include <iostream>
#include <cmath>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setMinimumSize(1080, 720);

    // Set up image manager, thumbnail loader and gallery
    imageManager = std::make_shared<ImageManager>();
    thumbnailLoader = std::make_shared<ThumbnailLoader>();

    gallery = new GalleryWidget(this);
    gallery->setManager(imageManager);
    gallery->setThumbnailLoader(thumbnailLoader);

    connect(gallery, &GalleryWidget::imageSelected, // image selected by user -> sent to image viewer for display and processing
            this, [this](std::shared_ptr<Image> img) {
                imageViewer->setImage(img);
            });

    // Set up image viewer
    imageViewer = new ImageViewer(this);
    //imageViewer->setGeometry(400, 50, 650, 600);


    // UI layout
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // Set up widgets for sections of the UI
    QWidget* fileWidget = new QWidget(this);
    QWidget* adjustmentWidget = new QWidget(this);

    fileWidget->setStyleSheet("background-color: lightgray;");
    adjustmentWidget->setStyleSheet("background-color: lightgreen;");
    gallery->setStyleSheet("background-color: lightyellow;");
    imageViewer->setStyleSheet("background-color: lightblue;");

    // Set size policies for the sections
    fileWidget->setFixedWidth(200);
    fileWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    adjustmentWidget->setFixedWidth(200);
    adjustmentWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    gallery->setMinimumHeight(75);
    gallery->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    imageViewer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // UI layouts
    QVBoxLayout* fileLayout = new QVBoxLayout;
    fileLayout->addWidget(fileWidget);

    QVBoxLayout* adjustmentLayout = new QVBoxLayout;
    adjustmentLayout->addWidget(adjustmentWidget);

    QVBoxLayout* imageLayout = new QVBoxLayout;
    QSplitter* splitter = new QSplitter(Qt::Vertical);
    splitter->addWidget(gallery);
    splitter->addWidget(imageViewer);
    //splitter->setChildrenCollapsible(false);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    //splitter->setSizes({180, 540});
    imageLayout->addWidget(splitter);

    // Add the layouts to the main layout
    QHBoxLayout* mainLayout = new QHBoxLayout();
    mainLayout->addLayout(fileLayout);
    mainLayout->addLayout(imageLayout);
    mainLayout->addLayout(adjustmentLayout);

    centralWidget->setLayout(mainLayout);
    

    
    btn = new QPushButton("Load images", fileWidget);
    //btn->setGeometry(100, 100, 200, 50);
    btn->setToolTip("Select a directory containing raw images to load into the gallery");

    
    slider = new QSlider(Qt::Horizontal, fileWidget);
    slider->setGeometry(0, 200, 100, 50);
    slider->setRange(0, 100);
    slider->setValue(50);

    /*
    pbar = new QProgressBar(fileWidget);
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
    btnGroup->addButton(rBtn3);*/

    /*pieMenu = new PieMenu(this);
    pieMenu->setButtonCount(5);*/

    QObject::connect(btn, &QPushButton::clicked, this, &MainWindow::onButtonClicked);
    //QObject::connect(btn, &QPushButton::pressed, pieMenu, &PieMenu::display);
    //QObject::connect(slider, &QSlider::valueChanged, pbar, &QProgressBar::setValue);
    QObject::connect(slider, &QSlider::valueChanged, imageViewer, &ImageViewer::onSliderChanged);

}

MainWindow::~MainWindow() {}

void MainWindow::onButtonClicked()
{
    btn->setText("Clicked!!!");

    imageManager->loadGroup(this);  // prompt user to select image directory
    gallery->update();              // refresh gallery after image list changed
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
        std::cout << "Escape key pressed" << std::endl;

    if (event->key() == Qt::Key_0)
        imageViewer->setCompareMode(0);
    else if (event->key() == Qt::Key_1)
        imageViewer->setCompareMode(1);
    else if (event->key() == Qt::Key_2)
        imageViewer->setCompareMode(2);
    else if (event->key() == Qt::Key_3)
        imageViewer->setCompareMode(3);
}