#pragma once

#include "core/image.h"

#include <QObject>
#include <QtConcurrent>
#include <memory>

class ThumbnailLoader : public QObject {
    Q_OBJECT
public:
    explicit ThumbnailLoader(QObject* parent = nullptr) : QObject(parent) {}

    void requestThumbnail(std::shared_ptr<Image> img);

signals:
    void thumbnailReady();
};
