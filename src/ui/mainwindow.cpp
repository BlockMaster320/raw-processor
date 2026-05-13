#include "mainwindow.h"
#include "../core/imagemanager.h"
#include "gallerywidget.h"
#include "../core/utility.h"
#include "uiconstants.h"

#include <QHBoxLayout>
#include <QSplitter>
#include <QVBoxLayout>

#include <iostream>
#include <cmath>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setMinimumSize(1080, 720);

    // Set up image manager, thumbnail loader and gallery
    imageManager = std::make_shared<ImageManager>();
    thumbnailLoader = std::make_shared<ThumbnailLoader>();
    adjustmentCellManager = std::make_shared<AdjustmentCellManager>();

    gallery = new GalleryWidget(this);
    gallery->setManager(imageManager);
    gallery->setThumbnailLoader(thumbnailLoader);

    connect(gallery, &GalleryWidget::imageSelected, // image selected by user -> sent to image viewer for display and processing
            this, [this](std::shared_ptr<Image> img) {
                //adjustmentWidget->setImage(nullptr);  // Clear adjustment cell widgets before the old image is potentially destroyed, then set the new image on both viewer (creates default cells) and adjustment UI.
                imageViewer->setImage(img);
                adjustmentPanelWidget->setImage(img);
                adjustmentCellManager->setActiveImage(img);
            });
    connect(gallery, &GalleryWidget::selectedImagesChanged,
            this, [this](const std::vector<std::shared_ptr<Image>>& selectedImages) {
                adjustmentCellManager->clearSelection();
                for (const auto& img : selectedImages) {
                    adjustmentCellManager->selectImage(img, false);
                }
                if (exportBtn) {
                    exportBtn->setEnabled(!selectedImages.empty());
                }
            });

    // Set up image viewer
    imageViewer = new ImageViewer(this);
    imageViewer->setAdjustmentCellManager(adjustmentCellManager);
    exporter = std::make_unique<Exporter>(imageViewer);

    // UI layout
    QWidget* centralWidget = new QWidget(this);
    centralWidget->setStyleSheet("background-color: " + appBackgroundColor + ";");
    setCentralWidget(centralWidget);

    // Set up widgets for sections of the UI
    QWidget* fileWidget = new QWidget(this);
    adjustmentPanelWidget = new AdjustmentPanelWidget(this);
    adjustmentPanelWidget->setAdjustmentCellManager(adjustmentCellManager);
    adjustmentCellManagerWidget = new AdjustmentCellManagerWidget(adjustmentCellManager, this);

    // Set size policies for the sections
    fileWidget->setFixedWidth(300);
    fileWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    adjustmentPanelWidget->setFixedWidth(300);
    adjustmentPanelWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    gallery->setMinimumHeight(75);
    gallery->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    imageViewer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // UI layouts
    QVBoxLayout* fileInnerLayout = new QVBoxLayout;  // layout inside fileWidget
    fileInnerLayout->setContentsMargins(10, 8, 10, 10);
    fileInnerLayout->setAlignment(Qt::AlignTop);
    fileWidget->setLayout(fileInnerLayout);

    QVBoxLayout* fileLayout = new QVBoxLayout;       // file section
    fileLayout->addWidget(fileWidget);

    QVBoxLayout* adjustmentLayout = new QVBoxLayout; // adjustment section (cells for current image)
    adjustmentLayout->addWidget(adjustmentPanelWidget);

    QVBoxLayout* imageLayout = new QVBoxLayout;      // image display section
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

    btn = new QPushButton("Load images");
    btn->setStyleSheet(baseButtonStyle);
    btn->setToolTip("Select a directory containing raw images to load into the gallery");
    fileInnerLayout->addWidget(btn);

    exportBtn = new QPushButton("Export selected images");
    exportBtn->setStyleSheet(baseButtonStyle);
    exportBtn->setToolTip("Export the selected gallery images as JPEG files");
    exportBtn->setEnabled(false);
    fileInnerLayout->addWidget(exportBtn);

    fileInnerLayout->addWidget(adjustmentCellManagerWidget);

    QObject::connect(btn, &QPushButton::clicked, this, &MainWindow::onButtonClicked);
    QObject::connect(exportBtn, &QPushButton::clicked, this, [this]() {
        exporter->exportSelectedImages(this, adjustmentCellManager);
    });
    //QObject::connect(btn, &QPushButton::pressed, pieMenu, &PieMenu::display);
    QObject::connect(adjustmentPanelWidget, &AdjustmentPanelWidget::adjustmentChanged,
                     imageViewer, &ImageViewer::onAdjustmentChanged);
    QObject::connect(adjustmentPanelWidget, &AdjustmentPanelWidget::activeCellChanged,
                     this, [this](AdjustmentCell* cell) {
                         if (cell && cell->data) {
                             adjustmentCellManager->setActiveCell(cell->data);
                         } else {
                             adjustmentCellManager->setActiveCell(nullptr);
                         }
                         adjustmentCellManagerWidget->clearPresetSelection();
                         adjustmentCellManagerWidget->updateActiveCell();
                     });
    QObject::connect(adjustmentCellManagerWidget, &AdjustmentCellManagerWidget::presetActivated,
                     this, [this]() {
                         adjustmentPanelWidget->clearActiveCellSelection();
                     });
    QObject::connect(adjustmentCellManagerWidget, &AdjustmentCellManagerWidget::appliedToImages,
                     this, [this]() {
                         auto activeImage = adjustmentCellManager->getActiveImage();
                         if (activeImage) {
                             adjustmentPanelWidget->setImage(activeImage);
                             // Activate the newly added cell (always appended at the back)
                             if (!activeImage->adjustmentCells.empty()) {
                                 adjustmentPanelWidget->setActiveCell(&activeImage->adjustmentCells.back());
                             }
                             imageViewer->onAdjustmentChanged();
                         }
                     });
    QObject::connect(adjustmentCellManager.get(), &AdjustmentCellManager::linkedCellDataChanged,
                     this, [this](QUuid cellDataId) {
                         adjustmentPanelWidget->updateCellVisualStates();
                         if (imageViewer && imageViewer->getCurrentImage() && imageViewer->getCurrentImage()->getIsLoaded()) {
                             imageViewer->onAdjustmentChanged();
                         }
                     });

}

MainWindow::~MainWindow() {}

void MainWindow::onButtonClicked()
{
    imageManager->loadGroup(this);  // prompt user to select image directory
    gallery->clearSelection();
    if (!imageManager->currentGroupPath.isEmpty()) {
        adjustmentCellManager->initialize(imageManager->currentGroupPath);
        adjustmentCellManagerWidget->updatePresetList();
        adjustmentCellManagerWidget->updateActiveCell();
    }
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
