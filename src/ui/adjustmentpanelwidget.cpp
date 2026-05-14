#include "adjustmentpanelwidget.h"
#include "adjustmentcellwidget.h"
#include "uiconstants.h"
#include "../core/image.h"
#include "../core/adjustmentcell.h"
#include "../core/adjustmentmanager.h"

#include <QFrame>
#include <QPushButton>
#include <QApplication>
#include <QMouseEvent>
#include <algorithm>
#include <set>
#include <QUuid>

AdjustmentPanelWidget::AdjustmentPanelWidget(QWidget* parent) : QWidget(parent)
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 8, 10, 10);
    mainLayout->setSpacing(6);

    // "Add Adjustment Cell" button
    createCellButton = new QPushButton("Add Adjustment Cell", this);
    createCellButton->setStyleSheet(baseButtonStyle);
    createCellButton->setToolTip("Create a new adjustment cell for the active image");
    createCellButton->setCursor(Qt::PointingHandCursor);
    createCellButton->setEnabled(false);
    auto* createButtonRow = new QWidget(this);
    auto* createButtonRowLayout = new QHBoxLayout(createButtonRow);
    createButtonRowLayout->setContentsMargins(4, 0, 4, 0);
    createButtonRowLayout->setSpacing(0);
    createButtonRowLayout->addWidget(createCellButton);
    mainLayout->addWidget(createButtonRow);
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

    // Drop indicator: shown between cells during drag-to-reorder
    dropIndicator = new QFrame(container);
    dropIndicator->setFrameShape(QFrame::HLine);
    dropIndicator->setFixedHeight(2);
    dropIndicator->setStyleSheet("QFrame { background-color: " + appActiveHighlightColor + "; border: none; }");
    dropIndicator->hide();
}

void AdjustmentPanelWidget::setAdjustmentCellManager(std::shared_ptr<AdjustmentManager> acm) {
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
    createCellButton->setEnabled(currentImage != nullptr);

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
        connect(cellWidget, &AdjustmentCellWidget::cellRenameRequested,
                this, [this](AdjustmentCell* requestedCell, const QString& requestedName) {
                    if (!requestedCell || !currentImage) {
                        return;
                    }

                    const QString newName = requestedName.trimmed();
                    if (newName.isEmpty()) {
                        return;
                    }

                    std::shared_ptr<AdjustmentGroup> previouslyActiveData =
                        (activeCell && activeCell->data) ? activeCell->data : nullptr;

                    std::set<Image*> touchedImages;
                    auto renameInImage = [&](const std::shared_ptr<Image>& image, const QUuid& linkedId, bool linked) {
                        if (!image) {
                            return;
                        }

                        bool changed = false;
                        for (auto& cell : image->adjustmentCells) {
                            const bool matches = linked
                                ? (cell.data && cell.data->id == linkedId)
                                : (&cell == requestedCell);
                            if (!matches) {
                                continue;
                            }
                            if (cell.data) {
                                cell.data->name = newName;
                            }
                            changed = true;
                        }

                        if (changed) {
                            image->saveAdjustmentCells();
                            touchedImages.insert(image.get());
                        }
                    };

                    if (requestedCell->isLinked() && requestedCell->data) {
                        const QUuid linkedId = requestedCell->data->id;
                        requestedCell->data->name = newName;

                        // Keep preset names synchronized with linked cell-data name.
                        if (adjustmentCellManager && !linkedId.isNull()) {
                            for (const auto& preset : adjustmentCellManager->getLocalPresets()) {
                                if (preset && preset->data && preset->data->id == linkedId && preset->name != newName) {
                                    adjustmentCellManager->renamePreset(preset, newName);
                                }
                            }
                            for (const auto& preset : adjustmentCellManager->getGlobalPresets()) {
                                if (preset && preset->data && preset->data->id == linkedId && preset->name != newName) {
                                    adjustmentCellManager->renamePreset(preset, newName);
                                }
                            }
                        }

                        if (adjustmentCellManager && !linkedId.isNull()) {
                            adjustmentCellManager->updateCellData(linkedId);

                            for (const auto& image : adjustmentCellManager->getSelectedImages()) {
                                renameInImage(image, linkedId, true);
                            }

                            auto activeImage = adjustmentCellManager->getActiveImage();
                            if (activeImage && touchedImages.find(activeImage.get()) == touchedImages.end()) {
                                renameInImage(activeImage, linkedId, true);
                            }
                        } else {
                            renameInImage(currentImage, linkedId, true);
                        }
                    } else {
                        if (requestedCell->data) {
                            requestedCell->data->name = newName;
                        }
                        currentImage->saveAdjustmentCells();
                    }

                    setImage(currentImage);
                    if (previouslyActiveData) {
                        for (auto& cell : currentImage->adjustmentCells) {
                            if (cell.data == previouslyActiveData) {
                                setActiveCell(&cell);
                                break;
                            }
                        }
                    }
                    emit adjustmentChanged();
                });
        connect(cellWidget, &AdjustmentCellWidget::removeCellRequested,
                this, &AdjustmentPanelWidget::removeCell);
        connect(cellWidget, &AdjustmentCellWidget::dragInitiated,
                this, &AdjustmentPanelWidget::onCellDragInitiated);
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

                    auto detached = std::make_shared<AdjustmentGroup>(cell.data->name);
                    detached->isEnabled = cell.data->isEnabled;
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

void AdjustmentPanelWidget::updateCellVisualStates()
{
    for (int i = 0; i < containerLayout->count(); ++i) {
        auto* w = qobject_cast<AdjustmentCellWidget*>(containerLayout->itemAt(i)->widget());
        if (!w) {
            continue;
        }
        w->updateVisualState();
    }
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


// -- ADJUSTMENT CELL DRAG-AND-DROP REORDERING --

void AdjustmentPanelWidget::onCellDragInitiated(AdjustmentCellWidget* widget, QPoint globalPos)
{
    draggedWidget = widget;
    isDraggingCell = true;
    dropTargetIndex = -1;
    dropIndicator->raise();
    dropIndicator->show();
    updateDropIndicator(container->mapFromGlobal(globalPos));
    qApp->installEventFilter(this);
}

bool AdjustmentPanelWidget::eventFilter(QObject* watched, QEvent* event)
{
    if (!isDraggingCell) {
        return QWidget::eventFilter(watched, event);
    }

    if (event->type() == QEvent::MouseMove) {
        auto* me = static_cast<QMouseEvent*>(event);
        updateDropIndicator(container->mapFromGlobal(me->globalPosition().toPoint()));
    } else if (event->type() == QEvent::MouseButtonRelease) {
        auto* me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::LeftButton) {
            qApp->removeEventFilter(this);
            isDraggingCell = false;
            dropIndicator->hide();
            performDrop();
            draggedWidget = nullptr;
            return true;
        }
    }

    return QWidget::eventFilter(watched, event);
}

void AdjustmentPanelWidget::updateDropIndicator(const QPoint& posInContainer)
{
    // Collect geometry for each cell widget in layout order
    QVector<QPair<int, int>> cellGeoms;
    for (int i = 0; i < containerLayout->count(); ++i) {
        auto* w = qobject_cast<AdjustmentCellWidget*>(containerLayout->itemAt(i)->widget());
        if (!w) continue;
        cellGeoms.append({w->geometry().top(), w->geometry().bottom()});
    }

    // Determine which gap the cursor is in
    dropTargetIndex = cellGeoms.size();
    for (int i = 0; i < cellGeoms.size(); ++i) {
        int midY = (cellGeoms[i].first + cellGeoms[i].second) / 2;
        if (posInContainer.y() < midY) {
            dropTargetIndex = i;
            break;
        }
    }

    // Position the indicator line
    int indicatorY;
    if (cellGeoms.isEmpty()) {
        indicatorY = 4;
    } else if (dropTargetIndex == 0) {
        indicatorY = cellGeoms[0].first - 4;
    } else if (dropTargetIndex >= cellGeoms.size()) {
        indicatorY = cellGeoms.back().second + 4;
    } else {
        indicatorY = (cellGeoms[dropTargetIndex - 1].second + cellGeoms[dropTargetIndex].first) / 2;
    }

    dropIndicator->setGeometry(4, indicatorY, container->width() - 8, 2);
}

void AdjustmentPanelWidget::performDrop()
{
    if (!currentImage || !draggedWidget || dropTargetIndex < 0) return;

    // Find source index
    int sourceIdx = -1;
    int idx = 0;
    for (int i = 0; i < containerLayout->count(); ++i) {
        auto* w = qobject_cast<AdjustmentCellWidget*>(containerLayout->itemAt(i)->widget());
        if (!w) continue;
        if (w == draggedWidget) { sourceIdx = idx; break; }
        ++idx;
    }

    // Nothing to do if source not found or drop is in the same position
    if (sourceIdx < 0 || dropTargetIndex == sourceIdx || dropTargetIndex == sourceIdx + 1) return;

    // Save active cell data pointer so we can restore selection after rebuild
    std::shared_ptr<AdjustmentGroup> activeCellData =
        (activeCell && activeCell->data) ? activeCell->data : nullptr;

    auto& cells = currentImage->adjustmentCells;
    AdjustmentCell movedCell = cells[sourceIdx];
    cells.erase(cells.begin() + sourceIdx);
    int insertIdx = (dropTargetIndex > sourceIdx) ? dropTargetIndex - 1 : dropTargetIndex;
    cells.insert(cells.begin() + insertIdx, movedCell);

    currentImage->saveAdjustmentCells();
    setImage(currentImage);

    // Restore the previously active cell
    if (activeCellData) {
        for (auto& cell : currentImage->adjustmentCells) {
            if (cell.data == activeCellData) {
                setActiveCell(&cell);
                break;
            }
        }
    }

    emit adjustmentChanged();
}

