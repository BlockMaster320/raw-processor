#include "imagemanager.h"

#include <QFileDialog>
#include <QDir>

void ImageManager::loadGroup(QWidget *parent)
{
    QFileDialog dialog;
    dialog.setOption(QFileDialog::DontUseNativeDialog);
    dialog.setFileMode(QFileDialog::Directory);

    QString dir;
    if (dialog.exec()) {
        dir = dialog.selectedFiles().first();
    }
    if (dir.isEmpty())
        return;

    currentGroupPath = dir;

    qDebug() << "Selected directory:" << dir;
    QDir directory(dir);
    QStringList files = directory.entryList(
        {"*.ARW", "*.CR2", "*.NEF", "*.DNG"},
        QDir::Files
    );

    images.clear();

    for (const QString& file : files) {
        const QString filePath = directory.filePath(file);
        auto img = std::make_shared<Image>(filePath);
        images.push_back(img);
    }

    //parent->gallery->update();              // refresh gallery after image list changed
}