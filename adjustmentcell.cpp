#include "adjustmentcell.h"

AdjustmentCell::AdjustmentCell(const QString& instanceName)
    : instanceName(instanceName), visible(true) {
    // Create a new unlinked cell data with default adjustments
    data = std::make_shared<AdjustmentCellData>(instanceName);
    data->adjustments[AdjType::Denoise]    = std::make_unique<AdjDenoise>();
    data->adjustments[AdjType::Exposure]   = std::make_unique<AdjExposure>();
    data->adjustments[AdjType::Contrast]   = std::make_unique<AdjContrast>();
    data->adjustments[AdjType::Midpoint]   = std::make_unique<AdjMidpoint>();
    data->adjustments[AdjType::PopArt]     = std::make_unique<AdjPopArt>();
    data->adjustments[AdjType::WhiteBlack] = std::make_unique<AdjWhiteBlack>();
    data->adjustments[AdjType::Saturation] = std::make_unique<AdjSaturation>();

    // Display order: tone adjustments first, then denoise at the bottom
    displayOrder = {
        AdjType::Exposure,
        AdjType::Contrast,
        AdjType::Midpoint,
        AdjType::WhiteBlack,
        AdjType::Saturation,
        AdjType::PopArt,
        AdjType::Denoise,
    };

    // Process order: denoise first (on raw-like data), then tone adjustments
    processOrder = {
        AdjType::Denoise,
        AdjType::Exposure,
        AdjType::Contrast,
        AdjType::Midpoint,
        AdjType::PopArt,
        AdjType::WhiteBlack,
        AdjType::Saturation,
    };
}

AdjustmentCell::AdjustmentCell(std::shared_ptr<AdjustmentCellData> cellData, const QString& instanceName)
    : data(cellData), instanceName(instanceName), visible(true) {
    // If no data provided, create a new one with defaults
    if (!data) {
        data = std::make_shared<AdjustmentCellData>(instanceName);
        data->adjustments[AdjType::Denoise]    = std::make_unique<AdjDenoise>();
        data->adjustments[AdjType::Exposure]   = std::make_unique<AdjExposure>();
        data->adjustments[AdjType::Contrast]   = std::make_unique<AdjContrast>();
        data->adjustments[AdjType::Midpoint]   = std::make_unique<AdjMidpoint>();
        data->adjustments[AdjType::PopArt]     = std::make_unique<AdjPopArt>();
        data->adjustments[AdjType::WhiteBlack] = std::make_unique<AdjWhiteBlack>();
        data->adjustments[AdjType::Saturation] = std::make_unique<AdjSaturation>();
    }

    // Set display and process order
    displayOrder = {
        AdjType::Exposure,
        AdjType::Contrast,
        AdjType::Midpoint,
        AdjType::WhiteBlack,
        AdjType::Saturation,
        AdjType::PopArt,
        AdjType::Denoise,
    };

    processOrder = {
        AdjType::Denoise,
        AdjType::Exposure,
        AdjType::Contrast,
        AdjType::Midpoint,
        AdjType::PopArt,
        AdjType::WhiteBlack,
        AdjType::Saturation,
    };
}

bool AdjustmentCell::isLinked() const {
    return data && data->isLinked();
}

const std::unordered_map<AdjType, std::unique_ptr<Adjustment>>& AdjustmentCell::getAdjustments() const {
    static std::unordered_map<AdjType, std::unique_ptr<Adjustment>> empty;
    return data ? data->adjustments : empty;
}
