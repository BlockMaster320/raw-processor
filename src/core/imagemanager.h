#pragma once

#include "image.h"

#include <QWidget>

class ImageManager {
public:
    void loadCollection(QWidget* parent);

    std::vector<std::shared_ptr<Image>> images;
    QString localCollectionPath;
};