#pragma once

#include "../adjustment.h"
#include "../adjustmentcell.h"

#include <QWidget>

class AdjustmentCellWidget : public QWidget {
    Q_OBJECT
public:
    explicit AdjustmentCellWidget(AdjustmentCell* cell, QWidget* parent = nullptr);

    AdjustmentCell* getCell() const { return cell; }

signals:
    void adjustmentChanged();
    void removeCellRequested(AdjustmentCell* cell);

private:
    AdjustmentCell* cell;
};
