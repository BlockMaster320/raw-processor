#pragma once

#include "../adjustmentcell.h"

#include <memory>

#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QPushButton>
#include <QFrame>
#include <QPoint>

class Image;
class AdjustmentCellManager;
class AdjustmentCellWidget;

// UI widget that displays adjustment cells for the current image.
class AdjustmentPanelWidget : public QWidget {
    Q_OBJECT
public:
    explicit AdjustmentPanelWidget(QWidget* parent = nullptr);

    void setAdjustmentCellManager(std::shared_ptr<AdjustmentCellManager> acm);
    void setImage(std::shared_ptr<Image> image);
    void clearActiveCellSelection();
    void updateCellVisualStates();  // Update visual states of all cell widgets

signals:
    void adjustmentChanged();
    void activeCellChanged(AdjustmentCell* cell);

public:
    void setActiveCell(AdjustmentCell* cell);

private slots:
    void onCreateCellClicked();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void removeCell(AdjustmentCell* cell);
    void onCellDragInitiated(AdjustmentCellWidget* widget, QPoint globalPos);
    void updateDropIndicator(const QPoint& posInContainer);
    void performDrop();

    QPushButton*  createCellButton;
    QScrollArea*  scrollArea;
    QWidget*      container;
    QVBoxLayout*  containerLayout;

    QFrame*       dropIndicator = nullptr;
    AdjustmentCellWidget* draggedWidget = nullptr;
    int           dropTargetIndex = -1;
    bool          isDraggingCell = false;

    std::shared_ptr<Image> currentImage;
    AdjustmentCell* activeCell = nullptr;
    std::shared_ptr<AdjustmentCellManager> adjustmentCellManager;
};
