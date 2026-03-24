#include "mainwindow.h"
#include "utility.h"

#include <iostream>
#include <cmath>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setFixedSize(1080, 720);

    canvas = new ImageViewer(this);
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
    QObject::connect(slider, &QSlider::valueChanged, canvas, &ImageViewer::onSliderChanged);
}

MainWindow::~MainWindow() {}

void MainWindow::onButtonClicked()
{
    btn->setText("Clicked!!!");
    
    // Upload image to canvas
    std::shared_ptr<Image> image = std::make_shared<Image>("./textures/raw-img1.ARW");
    canvas->setImage(image);
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