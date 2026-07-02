#pragma once

#ifndef IMAGE_H
#define IMAGE_H

#include "adjustment.h"
#include "adjustmentcell.h"

#include <string>
#include <vector>

#include <QMatrix3x3>
#include <QPoint>
#include <QVector4D>
#include <QImage>

class AdjustmentManager;  // Forward declaration

class Image {
public:
    explicit Image(const QString& path);

    bool loadRawData();
    void loadAdjustmentCells(AdjustmentManager* acm = nullptr);
    void saveAdjustmentCells();

    // Getters & setters
    bool getIsLoaded() const;

    const uint16_t* getRawData() const;
    int getRawWidth() const;
    int getRawHeight() const;

    int getImageWidth() const;
    int getImageHeight() const;
    int getLeftMargin() const;
    int getTopMargin() const;
    QPoint getBayerOffset() const;
    const QVector4D& getBlackLevels() const;
    const QVector4D& getWbMultipliers() const;
    const QMatrix3x3& getCamToSrgb() const;
    const QMatrix3x3& getCamToXyz() const;
    const QMatrix3x3& getCamToRec2020() const;
    const QMatrix3x3& getRec2020ToSrgb() const;

    const std::vector<unsigned char>& getThumbnailBytes() const;

    // Reference libraw-processed image
    bool buildReferenceImage();
    const uint16_t* getReferenceData() const;
    int getReferenceWidth() const;
    int getReferenceHeight() const;

    QString imagePath;

    QImage thumbnail;
    std::atomic<bool> thumbnailLoaded = false;
    std::atomic<bool> thumbnailLoading = false;
    bool adjustmentCellsLoaded = false;

    std::vector<AdjustmentCell> adjustmentCells;

private:
    void clearLoadedData();
    QString getSidecarPath() const;

    std::vector<uint16_t> rawPixels;
    int rawWidth;
    int rawHeight;
    int imageWidth;
    int imageHeight;
    int leftMargin;
    int topMargin;
    QPoint bayerOffset;

    QVector4D blackLevels;
    QVector4D wbMultipliers;
    QMatrix3x3 camToSrgbMat;
    QMatrix3x3 camToXyzMat;
    QMatrix3x3 camToRec2020Mat;
    QMatrix3x3 rec2020ToSrgbMat;

    std::vector<uint16_t> referencePixels;
    int referenceWidth;
    int referenceHeight;

    bool isLoaded;
};

#endif // IMAGE_H