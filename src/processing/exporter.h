#pragma once

#include <memory>

class AdjustmentManager;
class ImageViewer;
class QWidget;

class Exporter {
public:
    explicit Exporter(ImageViewer* viewer);

    bool exportSelectedImages(QWidget* parent, const std::shared_ptr<AdjustmentManager>& adjustmentCellManager);

private:
    ImageViewer* viewer;
};