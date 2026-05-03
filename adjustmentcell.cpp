#include "adjustmentcell.h"

// Set ups default "Base" adjustment cell.
AdjustmentCell::AdjustmentCell(std::string name) : name(std::move(name)), isVisible(true)
{
    adjustments.emplace_back(std::make_unique<AdjExposure>());
    adjustments.emplace_back(std::make_unique<AdjContrast>());
    adjustments.emplace_back(std::make_unique<AdjMidpoint>());
    adjustments.emplace_back(std::make_unique<AdjPopArt>());
    adjustments.emplace_back(std::make_unique<AdjWhiteBlack>());
    adjustments.emplace_back(std::make_unique<AdjSaturation>());
    adjustments.emplace_back(std::make_unique<AdjDenoise>());
}
