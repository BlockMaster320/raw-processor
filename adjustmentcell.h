#pragma once

#include "adjustment.h"
#include "adjustmentcelldata.h"

#include <string>
#include <vector>
#include <memory>

#include <QString>

// Represents an instance of an adjustment cell that can optionally be linked
// to a shared AdjustmentCellData, allowing for dynamic linking across multiple images.
class AdjustmentCell {
public:
    explicit AdjustmentCell(const QString& instanceName = "Cell");
    explicit AdjustmentCell(std::shared_ptr<AdjustmentCellData> cellData, const QString& instanceName = "Cell");

    // Core data - can be shared with other cells for dynamic linking
    std::shared_ptr<AdjustmentCellData> data;

    // Instance-specific properties
    QString instanceName;
    bool visible = true;

    // Display and process order for adjustments
    std::vector<AdjType> displayOrder;  // order in which adjustments are shown in the UI
    std::vector<AdjType> processOrder;  // order in which adjustments are applied during rendering

    // Check if this cell is dynamically linked
    bool isLinked() const;

    // Legacy compatibility - access adjustments through data
    const std::unordered_map<AdjType, std::unique_ptr<Adjustment>>& getAdjustments() const;
};