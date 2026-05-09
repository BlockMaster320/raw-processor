#pragma once

#include "image.h"

#include <QWidget>

class ImageManager {
public:
    void loadGroup(QWidget* parent);

    std::vector<std::shared_ptr<Image>> images;
    QString currentGroupPath;
};