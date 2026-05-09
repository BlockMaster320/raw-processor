#pragma once

#include "PieMenu.h"
#include "../image.h"
#include "../imageprocessor.h"
#include "../adjustmentcellmanager.h"
#include "imageviewer.h"
#include "../imagemanager.h"
#include "gallerywidget.h"
#include "../thumbnailloader.h"
#include "adjustmentpanelwidget.h"
#include "adjustmentcellmanagerwidget.h"

#include <memory>

#include <QWidget>
#include <QMainWindow>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>
#include <QPushButton>
#include <QProgressBar>
#include <QSlider>
#include <QRadioButton>
#include <QButtonGroup>
#include <QKeyEvent>

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
    ImageViewer* imageViewer;
    GalleryWidget* gallery;
    AdjustmentPanelWidget* adjustmentPanelWidget;
    AdjustmentCellManagerWidget* adjustmentCellManagerWidget;
    std::shared_ptr<ImageManager> imageManager;
    std::shared_ptr<ThumbnailLoader> thumbnailLoader;
    std::shared_ptr<AdjustmentCellManager> adjustmentCellManager;

    QPushButton *btn;
    QProgressBar *pbar;
    QSlider *slider;
    QRadioButton *rBtn1;
    QRadioButton *rBtn2;
    QRadioButton *rBtn3;
    QButtonGroup *btnGroup;

    PieMenu *pieMenu;
};