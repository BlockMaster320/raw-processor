#pragma once

#include "adjustment.h"

#include <string>
#include <vector>

// Represents a collection of adjustments. Each cell corresponds to one AdjustmentCellWidget in the UI.
class AdjustmentCell {
public:
    explicit AdjustmentCell(std::string name = "Cell");

    std::string name;
    bool isVisible = true;
    std::vector<std::unique_ptr<Adjustment>> adjustments;
};