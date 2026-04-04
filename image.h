#pragma once

#ifndef IMAGE_H
#define IMAGE_H

#include <string>
#include <vector>

#include <QMatrix3x3>
#include <QVector4D>
#include <QImage>

class Image {
public:
    explicit Image(const QString& path);

    bool loadRawData();
    bool buildReferenceImage();

    // Getters & setters
    bool getIsLoaded() const;

    const uint16_t* getRawData() const;
    int getRawWidth() const;
    int getRawHeight() const;

    int getImageWidth() const;
    int getImageHeight() const;
    int getLeftMargin() const;
    int getTopMargin() const;
    const QVector4D& getBlackLevels() const;
    const QVector4D& getWbMultipliers() const;
    const QMatrix3x3& getCamToSrgb() const;
    const QMatrix3x3& getCamToXyz() const;

    const std::vector<unsigned char>& getThumbnailBytes() const;

    const uint16_t* getReferenceData() const;  // reference libraw-processed image
    int getReferenceWidth() const;
    int getReferenceHeight() const;


    QString imagePath;

    QImage thumbnail;
    std::atomic<bool> thumbnailLoaded = false;
    std::atomic<bool> thumbnailLoading = false;

private:
    void clearLoadedData();

    std::vector<uint16_t> rawPixels;
    int rawWidth;
    int rawHeight;
    int imageWidth;
    int imageHeight;
    int leftMargin;
    int topMargin;

    QVector4D blackLevels;
    QVector4D wbMultipliers;
    QMatrix3x3 camToSrgbMat;
    QMatrix3x3 camToXyzMat;

    std::vector<uint16_t> referencePixels;
    int referenceWidth;
    int referenceHeight;

    bool isLoaded;
};

#endif // IMAGE_H