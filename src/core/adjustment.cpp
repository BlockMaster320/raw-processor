#include "adjustment.h"
#include "processing/imageprocessor.h"

#include <cmath>

AdjAttribute::AdjAttribute(std::string name, float value, float min, float max, bool isAdjustable)
    : name(std::move(name)), value(value), min(min), max(max), isAdjustable(isAdjustable)
{}

AdjExposure::AdjExposure()
{
    attributes.emplace_back("Exposure", 0.0f, -5.0f, 5.0f, true); // range ±5 EV stops
}
void AdjExposure::apply(ImageProcessor& processor)
{
    if (std::abs(attributes[0].value) < 1e-6f) return;
    processor.uniforms.exposure += attributes[0].value;
    processor.uniformsDirty = true;
}

AdjContrast::AdjContrast()
{
    attributes.emplace_back("Contrast", 0.0f, -1.0f, 1.0f, true);
}
void AdjContrast::apply(ImageProcessor& processor)
{
    if (std::abs(attributes[0].value) < 1e-6f) return;
    processor.uniforms.contrast += attributes[0].value;
    processor.uniformsDirty = true;
}

AdjMidpoint::AdjMidpoint()
{
    attributes.emplace_back("Midpoint", 0.2f, 0.0f, 1.0f, true);
}
void AdjMidpoint::apply(ImageProcessor& processor)
{
    // Midpoint only has effect when contrast is applied.
    if (std::abs(processor.uniforms.contrast) < 1e-6f) return;
    processor.uniforms.midpoint = attributes[0].value;
    processor.uniformsDirty = true;
}

AdjPopArt::AdjPopArt()
{
    attributes.emplace_back("LSD", 0.0f, -1.0f, 1.0f, true);
}
void AdjPopArt::apply(ImageProcessor& processor)
{
    if (std::abs(attributes[0].value) < 1e-6f) return;
    processor.uniforms.popArt += attributes[0].value;
    processor.uniformsDirty = true;
}

AdjWhiteBlack::AdjWhiteBlack()
{
    attributes.emplace_back("White", 0.0f, -1.0f, 1.0f, true);
    attributes.emplace_back("Black", 0.0f, -1.0f, 1.0f, true);
}
void AdjWhiteBlack::apply(ImageProcessor& processor)
{
    if (std::abs(attributes[0].value) < 1e-6f && std::abs(attributes[1].value) < 1e-6f) return;
    processor.uniforms.white += attributes[0].value;
    processor.uniforms.black += attributes[1].value;
    processor.uniformsDirty = true;
}

AdjSaturation::AdjSaturation()
{
    attributes.emplace_back("Saturation", 0.0f, -1.0f, 1.0f, true);
}
void AdjSaturation::apply(ImageProcessor& processor)
{
    if (std::abs(attributes[0].value) < 1e-6f) return;
    processor.uniforms.saturation += attributes[0].value;
    processor.uniformsDirty = true;
}


AdjDenoise::AdjDenoise()
{
    attributes.emplace_back("Denoise", 0.0f, 0.0f, 1.0f, true);
}
void AdjDenoise::apply(ImageProcessor& processor)
{
    if (attributes[0].value == 0.0f) return;

    processor.renderGlobalAdjustments(); // apply all pending global adjustments before the filter

    ImageProcessor::FilterAdjUniforms denoiseUniforms;
    denoiseUniforms.setFloat("strength", attributes[0].value);

    ImageProcessor::FilterAdjUniforms chromaH = denoiseUniforms;
    ImageProcessor::FilterAdjUniforms chromaV = denoiseUniforms;
    chromaH.setInt("horizontal", 1);
    chromaV.setInt("horizontal", 0);

    // Render passes: convert to YCbCr -> apply strong chroma blur -> apply light luma blur -> convert back to RGB.
    processor.renderFilterPass(ImageProcessor::FilterPassType::RgbToYcbcr);
    processor.renderFilterPass(ImageProcessor::FilterPassType::GaussianChroma, chromaH);
    processor.renderFilterPass(ImageProcessor::FilterPassType::GaussianChroma, chromaV);
    processor.renderFilterPass(ImageProcessor::FilterPassType::BilateralLuma, denoiseUniforms);
    processor.renderFilterPass(ImageProcessor::FilterPassType::YcbcrToRgb);
}

// --- Clone implementations for deep copying ---

std::unique_ptr<Adjustment> AdjExposure::clone() const {
    auto cloned = std::make_unique<AdjExposure>();
    cloned->attributes = attributes;
    return cloned;
}

std::unique_ptr<Adjustment> AdjContrast::clone() const {
    auto cloned = std::make_unique<AdjContrast>();
    cloned->attributes = attributes;
    return cloned;
}

std::unique_ptr<Adjustment> AdjMidpoint::clone() const {
    auto cloned = std::make_unique<AdjMidpoint>();
    cloned->attributes = attributes;
    return cloned;
}

std::unique_ptr<Adjustment> AdjPopArt::clone() const {
    auto cloned = std::make_unique<AdjPopArt>();
    cloned->attributes = attributes;
    return cloned;
}

std::unique_ptr<Adjustment> AdjWhiteBlack::clone() const {
    auto cloned = std::make_unique<AdjWhiteBlack>();
    cloned->attributes = attributes;
    return cloned;
}

std::unique_ptr<Adjustment> AdjSaturation::clone() const {
    auto cloned = std::make_unique<AdjSaturation>();
    cloned->attributes = attributes;
    return cloned;
}

std::unique_ptr<Adjustment> AdjDenoise::clone() const {
    auto cloned = std::make_unique<AdjDenoise>();
    cloned->attributes = attributes;
    return cloned;
}
