#pragma once

#include "imagemanager.h"
#include "thumbnailloader.h"

#include <QRect>
#include <QWidget>

#include <memory>

class GalleryWidget : public QWidget {
    Q_OBJECT
public:
    explicit GalleryWidget(QWidget* parent = nullptr)
        : QWidget(parent), baseIndex(0) {}

    void setManager(std::shared_ptr<ImageManager> mgr);
    void setThumbnailLoader(std::shared_ptr<ThumbnailLoader> loader);

signals:
    void imageSelected(std::shared_ptr<Image> img);

protected:
    void paintEvent(QPaintEvent*) override;
    void wheelEvent(QWheelEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;

private:
    int cellExtent() const;
    int visibleCellCount() const;
    QRect cellRectAt(int visualIndex, int visibleCount) const;

    int baseIndex;

    static constexpr int kHorizontalGap = 8;
    static constexpr int kOuterPadding = 8;
    static constexpr int kVerticalPadding = 0;

    std::shared_ptr<ImageManager> manager;
    std::shared_ptr<ThumbnailLoader> thumbnailLoader;
};