#pragma once

#include <memory>

class AdjustmentCellManager;
class ImageViewer;
class QWidget;

class Exporter {
public:
    explicit Exporter(ImageViewer* viewer);

    bool exportSelectedImages(QWidget* parent, const std::shared_ptr<AdjustmentCellManager>& adjustmentCellManager);

private:
    ImageViewer* viewer;
};