#pragma once

#include "../adjustmentcell.h"

#include <memory>

#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>

class Image;

// UI widget that displays adjustment cells for the current image.
class AdjustmentPanelWidget : public QWidget {
    Q_OBJECT
public:
    explicit AdjustmentPanelWidget(QWidget* parent = nullptr);

    void setImage(std::shared_ptr<Image> image);

signals:
    void adjustmentChanged();

private:
    void removeCell(AdjustmentCell* cell);

    QScrollArea*  scrollArea;
    QWidget*      container;
    QVBoxLayout*  containerLayout;

    std::shared_ptr<Image> currentImage;
};
