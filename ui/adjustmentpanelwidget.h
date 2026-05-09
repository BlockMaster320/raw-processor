#pragma once

#include "../adjustmentcell.h"

#include <memory>

#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QPushButton>

class Image;
class AdjustmentCellManager;

// UI widget that displays adjustment cells for the current image.
class AdjustmentPanelWidget : public QWidget {
    Q_OBJECT
public:
    explicit AdjustmentPanelWidget(QWidget* parent = nullptr);

    void setAdjustmentCellManager(std::shared_ptr<AdjustmentCellManager> acm);
    void setImage(std::shared_ptr<Image> image);
    void clearActiveCellSelection();

signals:
    void adjustmentChanged();
    void activeCellChanged(AdjustmentCell* cell);

private slots:
    void onCreateCellClicked();

private:
    void setActiveCell(AdjustmentCell* cell);
    void removeCell(AdjustmentCell* cell);

    QPushButton*  createCellButton;
    QScrollArea*  scrollArea;
    QWidget*      container;
    QVBoxLayout*  containerLayout;

    std::shared_ptr<Image> currentImage;
    AdjustmentCell* activeCell = nullptr;
    std::shared_ptr<AdjustmentCellManager> adjustmentCellManager;
};
