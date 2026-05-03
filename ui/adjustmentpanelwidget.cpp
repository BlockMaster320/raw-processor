#include "adjustmentpanelwidget.h"
#include "adjustmentcellwidget.h"
#include "../image.h"
#include "../adjustmentcell.h"

#include <QFrame>
#include <algorithm>

AdjustmentPanelWidget::AdjustmentPanelWidget(QWidget* parent) : QWidget(parent)
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setFrameShape(QFrame::NoFrame);

    container = new QWidget();
    containerLayout = new QVBoxLayout(container);
    containerLayout->setContentsMargins(4, 4, 4, 4);
    containerLayout->setSpacing(8);
    containerLayout->addStretch();
    container->setLayout(containerLayout);

    scrollArea->setWidget(container);
    mainLayout->addWidget(scrollArea);
}

// Populate the adjustment widget with cells and sliders corresponding to the given image's adjustments.
void AdjustmentPanelWidget::setImage(std::shared_ptr<Image> image)
{
    // Remove all existing cell widgets
    while (containerLayout->count() > 0) {
        QLayoutItem* item = containerLayout->takeAt(0);
        if (item->widget())
            delete item->widget();
        delete item;
    }

    // currentImage = nullptr;

    // if (!image) {
    //     containerLayout->addStretch();
    //     return;
    // }

    currentImage = image;

    // Add a cell widget for each adjustment cell
    for (auto& cell : image->adjustmentCells) {
        auto* cellWidget = new AdjustmentCellWidget(&cell, container);
        connect(cellWidget, &AdjustmentCellWidget::adjustmentChanged,
                this, &AdjustmentPanelWidget::adjustmentChanged);
        connect(cellWidget, &AdjustmentCellWidget::removeCellRequested,
                this, &AdjustmentPanelWidget::removeCell);
        containerLayout->addWidget(cellWidget);
    }
    containerLayout->addStretch();
}


// Removes the given cell from the current image and deletes its corresponding widget.
void AdjustmentPanelWidget::removeCell(AdjustmentCell* cell)
{
    if (!currentImage) return;

    // Find and erase the cell from the image's list
    auto& cells = currentImage->adjustmentCells;
    cells.erase(std::remove_if(cells.begin(), cells.end(),
        [cell](const AdjustmentCell& c) { return &c == cell; }),
        cells.end());

    // Find and delete the corresponding widget
    for (int i = 0; i < containerLayout->count(); ++i) {
        auto* w = qobject_cast<AdjustmentCellWidget*>(containerLayout->itemAt(i)->widget());
        if (w && w->getCell() == cell) {
            containerLayout->removeWidget(w);
            delete w;
            break;
        }
    }

    emit adjustmentChanged();
}
