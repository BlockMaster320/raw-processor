#pragma once

#include <string>
#include <vector>
#include <memory>

class ImageProcessor;  // forward declaration — full type only needed in adjustment.cpp

// --- AdjAttribute ---

// Represents a single adjustable attribute within an Adjustment.
class AdjAttribute {
public:
    AdjAttribute(std::string name, float value, float min, float max, bool isAdjustable = true);

    std::string name;
    float value;
    float min;
    float max;
    bool isAdjustable;
};

// --- Adjustment ---

// Abstract base class for all adjustments.
class Adjustment {
public:
    virtual ~Adjustment() = default;
    virtual void apply(ImageProcessor& processor) = 0;

    std::vector<AdjAttribute> attributes;
};

// --- Global adjustments (accumulate into ImageProcessor::uniforms) ---

class AdjExposure : public Adjustment {
public:
    AdjExposure();
    void apply(ImageProcessor& processor) override;
};

class AdjContrast : public Adjustment {
public:
    AdjContrast();
    void apply(ImageProcessor& processor) override;
};

class AdjMidpoint : public Adjustment {
public:
    AdjMidpoint();
    void apply(ImageProcessor& processor) override;
};

class AdjPopArt : public Adjustment {
public:
    AdjPopArt();
    void apply(ImageProcessor& processor) override;
};

class AdjWhiteBlack : public Adjustment {
public:
    AdjWhiteBlack();
    void apply(ImageProcessor& processor) override;
};

class AdjSaturation : public Adjustment {
public:
    AdjSaturation();
    void apply(ImageProcessor& processor) override;
};


// --- Local adjustments ---

class AdjDenoise : public Adjustment {
public:
    AdjDenoise();
    void apply(ImageProcessor& processor) override;
};

