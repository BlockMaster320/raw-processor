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
    explicit AdjustmentCell(const QString& cellName = "Cell");
    explicit AdjustmentCell(std::shared_ptr<AdjustmentGroup> cellData);

    // Core data - can be shared with other cells for dynamic linking
    std::shared_ptr<AdjustmentGroup> data;

    // Instance-specific properties
    bool isVisible = true;
    bool isCollapsed = false;

    // Display and process order for adjustments
    std::vector<AdjType> displayOrder;  // order in which adjustments are shown in the UI
    std::vector<AdjType> processOrder;  // order in which adjustments are applied during rendering

    // Check if this cell is dynamically linked
    bool isLinked() const;

    // Legacy compatibility - access adjustments through data
    const std::unordered_map<AdjType, std::unique_ptr<Adjustment>>& getAdjustments() const;
};