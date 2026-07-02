#include "thumbnailloader.h"

#include "libraw/libraw.h"

#include <QImage>


// Loads JPEG thumbnail of the given image on a separate thread. 
void ThumbnailLoader::requestThumbnail(std::shared_ptr<Image> img) {
    if (img->thumbnailLoaded || img->thumbnailLoading)
        return;

    img->thumbnailLoading = true;

    QtConcurrent::run([this, img]() {
        LibRaw raw;

        int ret = raw.open_file(img->imagePath.toStdWString().c_str());
        if (ret != LIBRAW_SUCCESS)
            return;

        ret = raw.unpack_thumb();
        if (ret != LIBRAW_SUCCESS)
            return;

        libraw_processed_image_t* thumb = raw.dcraw_make_mem_thumb();
        if (!thumb)
            return;

        QImage qimg = QImage::fromData(thumb->data, thumb->data_size, "JPEG");
        raw.dcraw_clear_mem(thumb);
        img->thumbnail = qimg;
        img->thumbnailLoaded = true;
        img->thumbnailLoading = false;

        emit thumbnailReady();
    });
}