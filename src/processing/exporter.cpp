#include "exporter.h"

#include "core/adjustmentmanager.h"
#include "core/image.h"
#include "imageprocessor.h"
#include "ui/imageviewer.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QImage>
#include <QMessageBox>

#include <vector>

namespace {

// Generates a unique file path for the given directory and file name.
QString buildUniqueExportPath(const QString& directory, const QString& baseName)
{
    QDir dir(directory);
    QString candidate = dir.filePath(baseName + ".jpg");
    int suffix = 1;

    while (QFileInfo::exists(candidate)) {
        candidate = dir.filePath(QString("%1_%2.jpg").arg(baseName).arg(suffix++));
    }

    return candidate;
}

} // namespace

Exporter::Exporter(ImageViewer* viewer) : viewer(viewer) {}

// Exports the currently selected images to JPEG format, applying their adjustments.
bool Exporter::exportSelectedImages(QWidget* parent, const std::shared_ptr<AdjustmentManager>& adjustmentCellManager)
{
    if (!viewer || !adjustmentCellManager) {
        return false;
    }

    const auto& selectedImages = adjustmentCellManager->getSelectedImages();
    if (selectedImages.empty()) {
        QMessageBox::information(parent, "Export JPEGs", "Select one or more images before exporting.");
        return false;
    }

    const QString outputDirectory = QFileDialog::getExistingDirectory(parent, "Select export folder");
    if (outputDirectory.isEmpty()) return false;

    viewer->makeCurrent();  // activate the viewer's OpenGL context for processing
    ImageProcessor exportProcessor; // create a temporary instance of ImageProcessor for exporting (to avoid disrupting the viewer's state by using its processor)
    exportProcessor.initializeGL();

    std::vector<std::shared_ptr<Image>> imagesToExport(selectedImages.begin(), selectedImages.end());
    QStringList failures;
    int exportedCount = 0;

    for (const auto& image : imagesToExport) {
        if (!image) continue;

        if (!image->getIsLoaded())
            image->loadRawData();

        image->loadAdjustmentCells(adjustmentCellManager.get());

        std::vector<unsigned char> textureBuffer;
        int width = 0;
        int height = 0;
        if (!exportProcessor.processForExport(image, textureBuffer, width, height) || width <= 0 || height <= 0) {
            failures << QString("%1: failed to process image").arg(image->imagePath);
            continue;
        }

        const QString baseName = QFileInfo(image->imagePath).completeBaseName();
        const QString outputPath = buildUniqueExportPath(outputDirectory, baseName);

        QImage exportImage(textureBuffer.data(), width, height, width * 3, QImage::Format_RGB888);
        if (!exportImage.copy().save(outputPath, "JPEG", 95)) {
            failures << QString("%1: failed to save JPEG").arg(image->imagePath);
            continue;
        }

        ++exportedCount;
    }

    exportProcessor.cleanupGL();
    viewer->doneCurrent();

    if (exportedCount == 0) {
        QMessageBox::warning(parent, "Export JPEGs", failures.isEmpty() ? "No images were exported." : failures.join('\n'));
        return false;
    }

    if (!failures.isEmpty()) {
        QMessageBox::warning(parent, "Export JPEGs", QString("Exported %1 image(s), but some failed:\n%2").arg(exportedCount).arg(failures.join('\n')));
    } else {
        QMessageBox::information(parent, "Export JPEGs", QString("Successfully exported %1 image(s).\n").arg(exportedCount));
    }

    return true;
}
