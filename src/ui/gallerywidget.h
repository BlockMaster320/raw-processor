#pragma once

#include "../core/imagemanager.h"
#include "../core/thumbnailloader.h"
#include "../core/adjustmentmanager.h"

#include <QRect>
#include <QWidget>

#include <memory>
#include <set>
#include <vector>

class GalleryWidget : public QWidget {
    Q_OBJECT
public:
    explicit GalleryWidget(QWidget* parent = nullptr)
        : QWidget(parent), baseIndex(0), lastSelectedIndex(-1) {}

    void setManager(std::shared_ptr<ImageManager> mgr);
    void setThumbnailLoader(std::shared_ptr<ThumbnailLoader> loader);
    void setAdjustmentManager(std::shared_ptr<AdjustmentManager> mgr);
    void clearSelection();

signals:
    void imageSelected(std::shared_ptr<Image> img);
    void selectedImagesChanged(const std::vector<std::shared_ptr<Image>>& images);

protected:
    void paintEvent(QPaintEvent*) override;
    void wheelEvent(QWheelEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseDoubleClickEvent(QMouseEvent* e) override;

private:
    int cellExtent() const;
    int visibleCellCount() const;
    QRect cellRectAt(int visualIndex, int visibleCount) const;
    int indexAtPosition(const QPoint& pos) const;
    void emitSelectedImagesChanged();
    void pruneInvalidSelection();

    int baseIndex;
    int lastSelectedIndex;
    std::set<int> selectedIndices;

    static constexpr int kHorizontalGap = 8;
    static constexpr int kOuterPadding = 8;
    static constexpr int kVerticalPadding = 0;

    std::shared_ptr<ImageManager> imageManager;
    std::shared_ptr<ThumbnailLoader> thumbnailLoader;
    std::shared_ptr<AdjustmentManager> adjustmentManager;
};
