#include "gallerywidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>

#include <algorithm>

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
    manager = mgr;
    update();
}

void GalleryWidget::setThumbnailLoader(std::shared_ptr<ThumbnailLoader> loader)
{
    thumbnailLoader = loader;
    connect(loader.get(), &ThumbnailLoader::thumbnailReady, this, [this]() { update(); });
}

void GalleryWidget::paintEvent(QPaintEvent *)
{
    if (!manager) return;

    QPainter painter(this);
    const int visibleCount = visibleCellCount();
    const int maxIndex = std::max(0, static_cast<int>(manager->images.size()) - visibleCount);
    baseIndex = std::clamp(baseIndex, 0, maxIndex);

    for (int i = 0; i < visibleCount; i++) {
        int idx = baseIndex + i;
        if (idx >= static_cast<int>(manager->images.size())) break;

        auto img = manager->images[idx];

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
    }
}

void GalleryWidget::wheelEvent(QWheelEvent *e)
{
    int steps = e->angleDelta().y() / 120;
    baseIndex -= steps;

    if (manager) {
        const int visibleCount = visibleCellCount();
        int maxIndex = std::max(0, static_cast<int>(manager->images.size()) - visibleCount);
        baseIndex = std::clamp(baseIndex, 0, maxIndex);
    }

    update();
}

void GalleryWidget::mousePressEvent(QMouseEvent *e)
{
    if (!manager) return;

    const int visibleCount = visibleCellCount();
    if (visibleCount <= 0) return;

    for (int i = 0; i < visibleCount; ++i) {
        const QRect rect = cellRectAt(i, visibleCount);
        if (!rect.contains(e->pos())) continue;

        const int index = baseIndex + i;
        if (index >= 0 && index < static_cast<int>(manager->images.size())) {
            emit imageSelected(manager->images[index]);
        }
        return;
    }
}
