#pragma once

#include "../adjustment.h"
#include "../adjustmentcell.h"

#include <QWidget>
#include <QEvent>

class QMouseEvent;
class QLineEdit;

class AdjustmentCellWidget : public QWidget {
    Q_OBJECT
public:
    explicit AdjustmentCellWidget(AdjustmentCell* cell, QWidget* parent = nullptr);

    AdjustmentCell* getCell() const { return cell; }
    void setActive(bool isActive);
    void updateVisualState();  // Update visual state when cell data changes

signals:
    void adjustmentChanged();
    void sliderReleased(AdjustmentCell* cell);
    void cellActivated(AdjustmentCell* cell);
    void dragInitiated(AdjustmentCellWidget* widget, QPoint globalPos);
    void cellRenameRequested(AdjustmentCell* cell, const QString& newName);
    void removeCellRequested(AdjustmentCell* cell);
    void unlinkCellRequested(AdjustmentCell* cell);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void applyVisualState();

    AdjustmentCell* cell;
    bool isActive = false;
    bool isCollapsed = false;
    QPoint dragPressPos;
    bool trackingForDrag = false;
    QLineEdit* nameEdit = nullptr;
};
