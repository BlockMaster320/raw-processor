#include "gallerywidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>

#include <algorithm>

namespace {
bool imageSharesLinkedGroupWithActiveCell(const std::shared_ptr<Image>& image,
                                          const std::shared_ptr<AdjustmentManager>& adjustmentManager)
{
    if (!image || !adjustmentManager) {
        return false;
    }

    if (!image->adjustmentCellsLoaded) {
        image->loadAdjustmentCells(adjustmentManager.get());
    }

    auto activeCell = adjustmentManager->getActiveCell();
    if (!activeCell || !activeCell->isLinked()) {
        return false;
    }

    const QUuid activeId = activeCell->id;
    for (const auto& cell : image->adjustmentCells) {
        if (!cell.adjustmentGroup || !cell.adjustmentGroup->isLinked()) {
            continue;
        }
        if (cell.adjustmentGroup->id == activeId) {
            return true;
        }
    }

    return false;
}
}

int GalleryWidget::cellExtent() const
{
    return std::max(1, height() - 2 * kVerticalPadding);
}

int GalleryWidget::visibleCellCount() const
{
    const int usableWidth = std::max(0, width() - (2 * kOuterPadding));
    const int denominator = std::max(1, cellExtent() + kHorizontalGap);
    return std::max(1, (usableWidth + kHorizontalGap) / denominator);
}

QRect GalleryWidget::cellRectAt(int visualIndex, int visibleCount) const
{
    if (visibleCount <= 0) return QRect();

    const int contentWidth = std::max(0, width() - (2 * kOuterPadding));
    const int totalGaps = std::max(0, visibleCount - 1) * kHorizontalGap;
    const int distributedWidth = std::max(1, (contentWidth - totalGaps) / visibleCount);
    const int cellWidth = std::max(cellExtent(), distributedWidth);
    const int x = kOuterPadding + visualIndex * (cellWidth + kHorizontalGap);

    return QRect(x,
                 kVerticalPadding,
                 cellWidth,
                 std::max(0, height() - 2 * kVerticalPadding));
}

void GalleryWidget::setManager(std::shared_ptr<ImageManager> mgr)
{
    imageManager = mgr;
    clearSelection();
    update();
}

void GalleryWidget::setThumbnailLoader(std::shared_ptr<ThumbnailLoader> loader)
{
    thumbnailLoader = loader;
    connect(loader.get(), &ThumbnailLoader::thumbnailReady, this, [this]() { update(); });
}

void GalleryWidget::setAdjustmentManager(std::shared_ptr<AdjustmentManager> mgr)
{
    adjustmentManager = mgr;
    if (adjustmentManager) {
        connect(adjustmentManager.get(), &AdjustmentManager::linkedCellDataChanged,
                this, [this](QUuid) { update(); });
    }
    update();
}

void GalleryWidget::paintEvent(QPaintEvent *)
{
    if (!imageManager) return;

    pruneInvalidSelection();

    QPainter painter(this);
    const int visibleCount = visibleCellCount();
    const int maxIndex = std::max(0, static_cast<int>(imageManager->images.size()) - visibleCount);
    baseIndex = std::clamp(baseIndex, 0, maxIndex);

    for (int i = 0; i < visibleCount; i++) {
        int idx = baseIndex + i;
        if (idx >= static_cast<int>(imageManager->images.size())) break;

        auto img = imageManager->images[idx];

        QRect rect = cellRectAt(i, visibleCount);

        if (img->thumbnailLoaded && !img->thumbnail.isNull()) {
            const QSize targetSize = img->thumbnail.size().scaled(rect.size(), Qt::KeepAspectRatio);
            const QPoint topLeft(rect.center().x() - (targetSize.width() / 2),
                                 rect.center().y() - (targetSize.height() / 2));
            const QRect targetRect(topLeft, targetSize);
            painter.drawImage(targetRect, img->thumbnail);
        } else {
            painter.fillRect(rect, Qt::blue);
            thumbnailLoader->requestThumbnail(img);
        }

        if (selectedIndices.find(idx) != selectedIndices.end()) {
            painter.fillRect(rect, QColor(90, 170, 255, 90));
            QPen pen(QColor(90, 170, 255, 220));
            pen.setWidth(2);
            painter.setPen(pen);
            painter.drawRect(rect.adjusted(1, 1, -1, -1));
        }

        // Draw a small dot indicator if the image shares a linked adjustment group with the active cell.
        if (imageSharesLinkedGroupWithActiveCell(img, adjustmentManager)) {
            const int dotRadius = 5;
            const QPoint center(rect.center().x(), rect.bottom() - dotRadius - 4);

            painter.setRenderHint(QPainter::Antialiasing, true);
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(79, 169, 255, 245));
            painter.drawEllipse(center, dotRadius, dotRadius);

            QPen outline(QColor(220, 240, 255, 230));
            outline.setWidth(1);
            painter.setPen(outline);
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(center, dotRadius, dotRadius);
            painter.setRenderHint(QPainter::Antialiasing, false);
        }
    }
}

void GalleryWidget::wheelEvent(QWheelEvent *e)
{
    int steps = e->angleDelta().y() / 120;
    baseIndex -= steps;

    if (imageManager) {
        const int visibleCount = visibleCellCount();
        int maxIndex = std::max(0, static_cast<int>(imageManager->images.size()) - visibleCount);
        baseIndex = std::clamp(baseIndex, 0, maxIndex);
    }

    update();
}

void GalleryWidget::mousePressEvent(QMouseEvent *e)
{
    if (!imageManager || e->button() != Qt::LeftButton) return;

    const int index = indexAtPosition(e->pos());
    if (index < 0 || index >= static_cast<int>(imageManager->images.size())) {
        return;
    }

    const Qt::KeyboardModifiers mods = e->modifiers();
    if ((mods & Qt::ShiftModifier) && lastSelectedIndex >= 0) {
        const int start = std::min(lastSelectedIndex, index);
        const int end = std::max(lastSelectedIndex, index);
        for (int i = start; i <= end; ++i) {
            selectedIndices.insert(i);
        }
    } else if (mods & Qt::ControlModifier) {
        selectedIndices.insert(index);
    } else {
        selectedIndices.clear();
        selectedIndices.insert(index);
    }

    lastSelectedIndex = index;
    emitSelectedImagesChanged();
    update();
}

void GalleryWidget::mouseDoubleClickEvent(QMouseEvent* e)
{
    if (!imageManager || e->button() != Qt::LeftButton) {
        return;
    }

    const int index = indexAtPosition(e->pos());
    if (index >= 0 && index < static_cast<int>(imageManager->images.size())) {
        emit imageSelected(imageManager->images[index]);
    }
}

void GalleryWidget::clearSelection()
{
    selectedIndices.clear();
    lastSelectedIndex = -1;
    emitSelectedImagesChanged();
    update();
}

int GalleryWidget::indexAtPosition(const QPoint& pos) const
{
    if (!imageManager) {
        return -1;
    }

    const int visibleCount = visibleCellCount();
    if (visibleCount <= 0) {
        return -1;
    }

    for (int i = 0; i < visibleCount; ++i) {
        const QRect rect = cellRectAt(i, visibleCount);
        if (rect.contains(pos)) {
            return baseIndex + i;
        }
    }

    return -1;
}

void GalleryWidget::emitSelectedImagesChanged()
{
    std::vector<std::shared_ptr<Image>> selected;
    if (imageManager) {
        selected.reserve(selectedIndices.size());
        for (int idx : selectedIndices) {
            if (idx >= 0 && idx < static_cast<int>(imageManager->images.size())) {
                selected.push_back(imageManager->images[idx]);
            }
        }
    }

    emit selectedImagesChanged(selected);
}

void GalleryWidget::pruneInvalidSelection()
{
    if (!imageManager) {
        if (!selectedIndices.empty()) {
            selectedIndices.clear();
            lastSelectedIndex = -1;
            emitSelectedImagesChanged();
        }
        return;
    }

    const int size = static_cast<int>(imageManager->images.size());
    bool changed = false;
    for (auto it = selectedIndices.begin(); it != selectedIndices.end();) {
        if (*it < 0 || *it >= size) {
            it = selectedIndices.erase(it);
            changed = true;
        } else {
            ++it;
        }
    }

    if (lastSelectedIndex < 0 || lastSelectedIndex >= size) {
        lastSelectedIndex = selectedIndices.empty() ? -1 : *selectedIndices.rbegin();
    }

    if (changed) {
        emitSelectedImagesChanged();
    }
}
