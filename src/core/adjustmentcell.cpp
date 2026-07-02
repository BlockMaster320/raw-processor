#include "adjustmentcell.h"

AdjustmentCell::AdjustmentCell(const QString& cellName, std::shared_ptr<AdjustmentGroup> adjustmentGroup)
    : adjustmentGroup(std::move(adjustmentGroup)), isVisible(true) {
    // If no shared group is provided, create a new unlinked group with defaults.
    if (!this->adjustmentGroup) {
        this->adjustmentGroup = std::make_shared<AdjustmentGroup>(cellName);
        this->adjustmentGroup->adjustments[AdjType::Denoise]    = std::make_unique<AdjDenoise>();
        this->adjustmentGroup->adjustments[AdjType::WhiteBalance] = std::make_unique<AdjWhiteBalance>();
        this->adjustmentGroup->adjustments[AdjType::Exposure]   = std::make_unique<AdjExposure>();
        this->adjustmentGroup->adjustments[AdjType::Contrast]   = std::make_unique<AdjContrast>();
        this->adjustmentGroup->adjustments[AdjType::Midpoint]   = std::make_unique<AdjMidpoint>();
        // data->adjustments[AdjType::PopArt]     = std::make_unique<AdjPopArt>();
        // data->adjustments[AdjType::WhiteBlack] = std::make_unique<AdjWhiteBlack>();
        this->adjustmentGroup->adjustments[AdjType::Saturation] = std::make_unique<AdjSaturation>();
        this->adjustmentGroup->adjustments[AdjType::Vignette] = std::make_unique<AdjVignette>();
    }

    // Set display and process order
    displayOrder = {
        AdjType::WhiteBalance,
        AdjType::Exposure,
        AdjType::Contrast,
        AdjType::Midpoint,
        // AdjType::WhiteBlack,
        AdjType::Saturation,
        // AdjType::PopArt,
        AdjType::Denoise,
        AdjType::Vignette,
    };

    processOrder = {
        AdjType::Denoise,
        AdjType::WhiteBalance,
        AdjType::Exposure,
        AdjType::Contrast,
        AdjType::Midpoint,
        // AdjType::PopArt,
        // AdjType::WhiteBlack,
        AdjType::Saturation,
        AdjType::Vignette,
    };
}

// Check whether the cell is dynamically linked
bool AdjustmentCell::isLinked() const {
    return adjustmentGroup && adjustmentGroup->isLinked();
}

const std::unordered_map<AdjType, std::unique_ptr<Adjustment>>& AdjustmentCell::getAdjustments() const {
    static std::unordered_map<AdjType, std::unique_ptr<Adjustment>> empty;
    return adjustmentGroup ? adjustmentGroup->adjustments : empty;
}
