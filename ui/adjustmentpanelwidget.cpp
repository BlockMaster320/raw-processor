#include "adjustmentpanelwidget.h"
#include "adjustmentcellwidget.h"
#include "uiconstants.h"
#include "../image.h"
#include "../adjustmentcell.h"
#include "../adjustmentcellmanager.h"

#include <QFrame>
#include <QPushButton>
#include <algorithm>
#include <QUuid>

AdjustmentPanelWidget::AdjustmentPanelWidget(QWidget* parent) : QWidget(parent)
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Create Cell button
    createCellButton = new QPushButton("Create Cell", this);
    createCellButton->setStyleSheet(baseButtonStyle);
    mainLayout->addWidget(createCellButton);
    connect(createCellButton, &QPushButton::clicked, this, &AdjustmentPanelWidget::onCreateCellClicked);

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

void AdjustmentPanelWidget::setAdjustmentCellManager(std::shared_ptr<AdjustmentCellManager> acm) {
    adjustmentCellManager = acm;
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
    activeCell = nullptr;

    if (!currentImage) {
        containerLayout->addStretch();
        return;
    }

    // Add a cell widget for each adjustment cell
    for (auto& cell : image->adjustmentCells) {
        auto* cellWidget = new AdjustmentCellWidget(&cell, container);
        connect(cellWidget, &AdjustmentCellWidget::adjustmentChanged,
                this, &AdjustmentPanelWidget::adjustmentChanged);
        connect(cellWidget, &AdjustmentCellWidget::sliderReleased,
                this, [this](AdjustmentCell* changedCell) {
                    if (currentImage) {
                        currentImage->saveAdjustmentCells();
                        if (adjustmentCellManager && changedCell && changedCell->isLinked()) {
                            adjustmentCellManager->notifyCellDataChanged(changedCell->data);
                        }
                    }
                });
        connect(cellWidget, &AdjustmentCellWidget::cellActivated,
                this, &AdjustmentPanelWidget::setActiveCell);
        connect(cellWidget, &AdjustmentCellWidget::removeCellRequested,
                this, &AdjustmentPanelWidget::removeCell);
        connect(cellWidget, &AdjustmentCellWidget::unlinkCellRequested,
                this, [this, cellWidget](AdjustmentCell* requestedCell) {
                    Q_UNUSED(requestedCell);

                    if (!currentImage) return;

                    int targetIndex = -1;
                    int cellWidgetIndex = 0;
                    for (int i = 0; i < containerLayout->count(); ++i) {
                        auto* w = qobject_cast<AdjustmentCellWidget*>(containerLayout->itemAt(i)->widget());
                        if (!w) {
                            continue;
                        }

                        if (w == cellWidget) {
                            targetIndex = cellWidgetIndex;
                            break;
                        }
                        ++cellWidgetIndex;
                    }

                    auto& cells = currentImage->adjustmentCells;
                    if (targetIndex < 0 || targetIndex >= static_cast<int>(cells.size())) {
                        return;
                    }

                    auto& cell = cells[targetIndex];
                    if (!cell.data || !cell.isLinked()) {
                        return;
                    }

                    auto detached = std::make_shared<AdjustmentCellData>(cell.data->name);
                    detached->enabled = cell.data->enabled;
                    detached->isGlobal = false;
                    detached->id = QUuid();
                    detached->adjustments = cell.data->cloneAdjustments();
                    cell.data = detached;

                    if (activeCell == &cell) {
                        emit activeCellChanged(activeCell);
                    }

                    currentImage->saveAdjustmentCells();
                    setImage(currentImage);
                    emit adjustmentChanged();
                });
        containerLayout->addWidget(cellWidget);
    }
    containerLayout->addStretch();

    if (!currentImage->adjustmentCells.empty()) {
        setActiveCell(&currentImage->adjustmentCells.front());
    }
}


// Removes the given cell from the current image and deletes its corresponding widget.
void AdjustmentPanelWidget::removeCell(AdjustmentCell* cell)
{
    Q_UNUSED(cell);

    if (!currentImage) return;

    // Identify the clicked cell by widget position in the layout.
    // This is robust even if stored cell pointers became stale after vector reallocation.
    auto* senderWidget = qobject_cast<AdjustmentCellWidget*>(sender());
    if (!senderWidget) return;

    int targetIndex = -1;
    int cellWidgetIndex = 0;
    for (int i = 0; i < containerLayout->count(); ++i) {
        auto* w = qobject_cast<AdjustmentCellWidget*>(containerLayout->itemAt(i)->widget());
        if (!w) {
            continue;
        }

        if (w == senderWidget) {
            targetIndex = cellWidgetIndex;
            break;
        }
        ++cellWidgetIndex;
    }

    auto& cells = currentImage->adjustmentCells;
    if (targetIndex < 0 || targetIndex >= static_cast<int>(cells.size())) {
        return;
    }

    cells.erase(cells.begin() + targetIndex);

    activeCell = nullptr;
    emit activeCellChanged(nullptr);

    currentImage->saveAdjustmentCells();

    // Rebuild widgets so all raw pointers reference current vector elements.
    setImage(currentImage);
    emit adjustmentChanged();
}

void AdjustmentPanelWidget::onCreateCellClicked() {
    if (!currentImage) return;

    // Create a new unlinked (static) adjustment cell
    AdjustmentCell newCell("Cell " + QString::number(currentImage->adjustmentCells.size() + 1));
    currentImage->adjustmentCells.push_back(newCell);

    currentImage->saveAdjustmentCells();

    // Rebuild widgets so previously created widgets don't keep stale pointers.
    setImage(currentImage);
    if (!currentImage->adjustmentCells.empty()) {
        setActiveCell(&currentImage->adjustmentCells.back());
    }

    emit adjustmentChanged();
}

void AdjustmentPanelWidget::setActiveCell(AdjustmentCell* cell)
{
    activeCell = cell;

    for (int i = 0; i < containerLayout->count(); ++i) {
        auto* w = qobject_cast<AdjustmentCellWidget*>(containerLayout->itemAt(i)->widget());
        if (!w) {
            continue;
        }

        w->setActive(w->getCell() == activeCell);
    }

    emit activeCellChanged(activeCell);
}

void AdjustmentPanelWidget::clearActiveCellSelection()
{
    activeCell = nullptr;

    for (int i = 0; i < containerLayout->count(); ++i) {
        auto* w = qobject_cast<AdjustmentCellWidget*>(containerLayout->itemAt(i)->widget());
        if (!w) {
            continue;
        }

        w->setActive(false);
    }
}
