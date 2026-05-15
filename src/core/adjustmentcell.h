#pragma once

#include "adjustment.h"
#include "adjustmentgroup.h"

#include <string>
#include <vector>
#include <memory>

#include <QString>

// Represents an instance of an adjustment cell.
class AdjustmentCell {
public:
    AdjustmentCell(const QString& cellName = "Cell", std::shared_ptr<AdjustmentGroup> adjustmentGroup = nullptr);

    std::shared_ptr<AdjustmentGroup> adjustmentGroup;

    bool isVisible = true;
    bool isCollapsed = false;

    std::vector<AdjType> displayOrder;  // order in which adjustments are shown in the UI
    std::vector<AdjType> processOrder;  // order in which adjustments are applied during rendering

    bool isLinked() const;

    // Legacy compatibility - access adjustments through data
    const std::unordered_map<AdjType, std::unique_ptr<Adjustment>>& getAdjustments() const;
};