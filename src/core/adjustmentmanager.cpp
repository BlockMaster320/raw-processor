#include "adjustmentmanager.h"
#include "image.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QDebug>

namespace {
std::shared_ptr<AdjustmentGroup> ensurePresetCellData(
    std::map<QUuid, std::shared_ptr<AdjustmentGroup>>& cellDataMap,
    const QUuid& dataId,
    const QString& presetName,
    bool isGlobal
) {
    if (dataId.isNull()) {
        return nullptr;
    }

    auto it = cellDataMap.find(dataId);
    if (it != cellDataMap.end()) {
        return it->second;
    }

    // Keep preset links valid even if the cells object is missing this entry.
    auto cellData = std::make_shared<AdjustmentGroup>(presetName);
    cellData->id = dataId;
    cellData->isGlobal = isGlobal;
    cellDataMap[dataId] = cellData;
    return cellData;
}
}

AdjustmentManager::AdjustmentManager()
    : applyMode(ApplyMode::Copy), presetListMode(PresetScope::Local), idCounter(0) {}

AdjustmentManager::~AdjustmentManager() {}

void AdjustmentManager::initialize(const QString& localGroupPath) {
    // Reset in-memory state before loading from disk to avoid duplicate entries.
    adjustmentDataMap.clear();
    localPresets.clear();
    globalPresets.clear();
    activeCell.reset();
    activeImage.reset();
    selectedImages.clear();

    localFilePath = QDir(localGroupPath).filePath(".adjustment_cells.json");
    // Global file path should be in a centralized location (e.g., AppData)
    globalFilePath = QDir(QDir::homePath()).filePath(".raw-processor/global_presets.json");
    
    // Ensure global directory exists
    QDir globalDir = QDir::home();
    if (!globalDir.exists(".raw-processor")) {
        globalDir.mkdir(".raw-processor");
    }

    loadData(localGroupPath);
}

void AdjustmentManager::loadData(const QString& localGroupPath) {
    loadLocalData();
    loadGlobalData();
}

void AdjustmentManager::loadLocalData() {
    if (localFilePath.isEmpty()) return;

    QFile file(localFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open local adjustment cells file:" << localFilePath;
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) return;

    QJsonObject root = doc.object();

    // Load cell data from ID-keyed cells object
    if (root.contains("cells") && root["cells"].isObject()) {
        QJsonObject cellsObj = root["cells"].toObject();
        for (const auto& idStr : cellsObj.keys()) {
            QJsonValue cellValue = cellsObj.value(idStr);
            if (!cellValue.isObject()) continue;

            QJsonObject cellObj = cellValue.toObject();
            auto cellData = std::make_shared<AdjustmentGroup>();
            cellData->fromJson(cellObj);

            // Ensure ID matches the key
            QUuid id = QUuid(idStr);
            if (!id.isNull()) {
                cellData->id = id;
            } else if (cellData->id.isNull()) {
                cellData->id = generateId();
            }

            adjustmentDataMap[cellData->id] = cellData;
        }
    } else if (root.contains("cellData") && root["cellData"].isArray()) {
        // Backward compatibility: migrate old array format to ID-keyed format
        QJsonArray cellArray = root["cellData"].toArray();
        for (const auto& cellJson : cellArray) {
            if (!cellJson.isObject()) continue;

            QJsonObject cellObj = cellJson.toObject();
            auto cellData = std::make_shared<AdjustmentGroup>();
            cellData->fromJson(cellObj);

            if (cellData->id.isNull()) {
                cellData->id = generateId();
            }

            adjustmentDataMap[cellData->id] = cellData;
        }
        // Save in new format
        saveLocalData();
    }

    // Load local presets
    if (root.contains("presets") && root["presets"].isArray()) {
        QJsonArray presetArray = root["presets"].toArray();
        for (const auto& presetJson : presetArray) {
            if (!presetJson.isObject()) continue;

            QJsonObject presetObj = presetJson.toObject();
            QString name = presetObj.value("name").toString("Unnamed Preset");
            QString idStr = presetObj.value("dataId").toString();
            QUuid dataId = idStr.isEmpty() ? QUuid() : QUuid(idStr);
            auto presetData = ensurePresetCellData(adjustmentDataMap, dataId, name, false);

            auto preset = std::make_shared<Preset>(presetData, name, PresetScope::Local);
            localPresets.push_back(preset);
        }
    }
}

void AdjustmentManager::loadGlobalData() {
    if (globalFilePath.isEmpty()) return;

    QFile file(globalFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "No global adjustment cells file found:" << globalFilePath;
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) return;

    QJsonObject root = doc.object();

    // Load cell data from ID-keyed cells object
    if (root.contains("cells") && root["cells"].isObject()) {
        QJsonObject cellsObj = root["cells"].toObject();
        for (const auto& idStr : cellsObj.keys()) {
            QJsonValue cellValue = cellsObj.value(idStr);
            if (!cellValue.isObject()) continue;

            QJsonObject cellObj = cellValue.toObject();
            auto cellData = std::make_shared<AdjustmentGroup>();
            cellData->fromJson(cellObj);
            cellData->isGlobal = true;

            // Ensure ID matches the key
            QUuid id = QUuid(idStr);
            if (!id.isNull()) {
                cellData->id = id;
            } else if (cellData->id.isNull()) {
                cellData->id = generateId();
            }

            adjustmentDataMap[cellData->id] = cellData;
        }
    } else if (root.contains("cellData") && root["cellData"].isArray()) {
        // Backward compatibility: migrate old array format
        QJsonArray cellArray = root["cellData"].toArray();
        for (const auto& cellJson : cellArray) {
            if (!cellJson.isObject()) continue;

            QJsonObject cellObj = cellJson.toObject();
            auto cellData = std::make_shared<AdjustmentGroup>();
            cellData->fromJson(cellObj);
            cellData->isGlobal = true;

            if (cellData->id.isNull()) {
                cellData->id = generateId();
            }

            adjustmentDataMap[cellData->id] = cellData;
        }
        // Save in new format
        saveGlobalData();
    }

    // Load global presets
    if (root.contains("presets") && root["presets"].isArray()) {
        QJsonArray presetArray = root["presets"].toArray();
        for (const auto& presetJson : presetArray) {
            if (!presetJson.isObject()) continue;

            QJsonObject presetObj = presetJson.toObject();
            QString name = presetObj.value("name").toString("Unnamed Preset");
            QString idStr = presetObj.value("dataId").toString();
            QUuid dataId = idStr.isEmpty() ? QUuid() : QUuid(idStr);
            auto presetData = ensurePresetCellData(adjustmentDataMap, dataId, name, true);

            auto preset = std::make_shared<Preset>(presetData, name, PresetScope::Global);
            globalPresets.push_back(preset);
        }
    }
}

void AdjustmentManager::saveData() {
    saveLocalData();
    saveGlobalData();
}

void AdjustmentManager::saveLocalData() {
    if (localFilePath.isEmpty()) return;

    QJsonObject root;

    // Save local cell data as ID-keyed object
    QJsonObject cellsObj;
    for (const auto& [id, cellData] : adjustmentDataMap) {
        if (cellData->isGlobal) continue;  // Skip global cells
        cellsObj[id.toString()] = cellData->toJson();
    }
    root["cells"] = cellsObj;

    // Save local presets
    QJsonArray presetArray;
    for (const auto& preset : localPresets) {
        if (!preset || !preset->data || preset->data->id.isNull()) {
            continue;
        }
        QJsonObject presetObj;
        presetObj["dataId"] = preset->data->id.toString();
        presetObj["name"] = preset->name;
        presetArray.append(presetObj);
    }
    root["presets"] = presetArray;

    QFile file(localFilePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Could not save local adjustment cells to:" << localFilePath;
        return;
    }

    QJsonDocument doc(root);
    file.write(doc.toJson());
    file.close();
}

void AdjustmentManager::saveGlobalData() {
    if (globalFilePath.isEmpty()) return;

    QJsonObject root;

    // Save global cell data as ID-keyed object
    QJsonObject cellsObj;
    for (const auto& [id, cellData] : adjustmentDataMap) {
        if (!cellData->isGlobal) continue;  // Skip local cells
        cellsObj[id.toString()] = cellData->toJson();
    }
    root["cells"] = cellsObj;

    // Save global presets
    QJsonArray presetArray;
    for (const auto& preset : globalPresets) {
        if (!preset || !preset->data || preset->data->id.isNull()) {
            continue;
        }
        QJsonObject presetObj;
        presetObj["dataId"] = preset->data->id.toString();
        presetObj["name"] = preset->name;
        presetArray.append(presetObj);
    }
    root["presets"] = presetArray;

    QFile file(globalFilePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Could not save global adjustment cells to:" << globalFilePath;
        return;
    }

    QJsonDocument doc(root);
    file.write(doc.toJson());
    file.close();
}

void AdjustmentManager::updateCellData(const QUuid& id) {
    // Update the cell in the appropriate file
    auto it = adjustmentDataMap.find(id);
    if (it != adjustmentDataMap.end() && it->second->isGlobal) {
        saveGlobalData();
    } else {
        saveLocalData();
    }
}

std::shared_ptr<AdjustmentGroup> AdjustmentManager::getCellData(const QUuid& id) const {
    auto it = adjustmentDataMap.find(id);
    return it != adjustmentDataMap.end() ? it->second : nullptr;
}

void AdjustmentManager::registerCellData(std::shared_ptr<AdjustmentGroup> cellData) {
    if (cellData && !cellData->id.isNull()) {
        adjustmentDataMap[cellData->id] = cellData;
    }
}

void AdjustmentManager::refreshCellData(const QUuid& id) {
    if (id.isNull()) return;

    // Try to reload from local file first
    if (!localFilePath.isEmpty()) {
        QFile file(localFilePath);
        if (file.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            file.close();

            if (doc.isObject()) {
                QJsonObject root = doc.object();
                if (root.contains("cells") && root["cells"].isObject()) {
                    QJsonObject cellsObj = root["cells"].toObject();
                    QString idStr = id.toString();
                    if (cellsObj.contains(idStr)) {
                        QJsonObject cellObj = cellsObj.value(idStr).toObject();
                        auto it = adjustmentDataMap.find(id);
                        if (it != adjustmentDataMap.end()) {
                            // Update existing cell data from file
                            it->second->fromJson(cellObj);
                        }
                        return;
                    }
                }
            }
        }
    }

    // If not in local file, try global file
    if (!globalFilePath.isEmpty()) {
        QFile file(globalFilePath);
        if (file.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            file.close();

            if (doc.isObject()) {
                QJsonObject root = doc.object();
                if (root.contains("cells") && root["cells"].isObject()) {
                    QJsonObject cellsObj = root["cells"].toObject();
                    QString idStr = id.toString();
                    if (cellsObj.contains(idStr)) {
                        QJsonObject cellObj = cellsObj.value(idStr).toObject();
                        auto it = adjustmentDataMap.find(id);
                        if (it != adjustmentDataMap.end()) {
                            // Update existing cell data from file
                            it->second->fromJson(cellObj);
                        }
                    }
                }
            }
        }
    }
}

void AdjustmentManager::setActiveCell(std::shared_ptr<AdjustmentGroup> cellData) {
    activeCell = cellData;
}

std::shared_ptr<AdjustmentGroup> AdjustmentManager::getActiveCell() const {
    return activeCell;
}

void AdjustmentManager::setActiveImage(std::shared_ptr<Image> image) {
    activeImage = image;
}

std::shared_ptr<Image> AdjustmentManager::getActiveImage() const {
    return activeImage;
}

void AdjustmentManager::selectImage(std::shared_ptr<Image> image, bool clearSelection) {
    if (clearSelection) {
        selectedImages.clear();
    }
    auto it = findImageInSelection(image);
    if (it == selectedImages.end()) {
        selectedImages.push_back(image);
    }
}

void AdjustmentManager::addToSelection(std::shared_ptr<Image> image) {
    auto it = findImageInSelection(image);
    if (it != selectedImages.end()) {
        selectedImages.erase(it);  // Toggle selection
    } else {
        selectedImages.push_back(image);
    }
}

void AdjustmentManager::addRangeToSelection(std::shared_ptr<Image> fromImage, std::shared_ptr<Image> toImage) {
    // This would require knowing the order of images, typically managed by gallery
    // For now, just add individual images
    addToSelection(fromImage);
    addToSelection(toImage);
}

const std::list<std::shared_ptr<Image>>& AdjustmentManager::getSelectedImages() const {
    return selectedImages;
}

void AdjustmentManager::clearSelection() {
    selectedImages.clear();
}

void AdjustmentManager::setApplyMode(ApplyMode mode) {
    applyMode = mode;
}

AdjustmentManager::ApplyMode AdjustmentManager::getApplyMode() const {
    return applyMode;
}

void AdjustmentManager::setPresetListMode(PresetScope scope) {
    presetListMode = scope;
}

PresetScope AdjustmentManager::getPresetListMode() const {
    return presetListMode;
}

void AdjustmentManager::createPreset(const QString& presetName, bool isGlobal) {
    if (!activeCell) return;

    // Create a new linked cell data for the preset
    auto presetData = std::make_shared<AdjustmentGroup>(presetName);
    presetData->id = generateId();
    presetData->isGlobal = isGlobal;
    presetData->isEnabled = activeCell->isEnabled;

    // Deep copy adjustments from active cell
    presetData->adjustments = activeCell->cloneAdjustments();

    adjustmentDataMap[presetData->id] = presetData;

    auto preset = std::make_shared<Preset>(presetData, presetName,
                                          isGlobal ? PresetScope::Global : PresetScope::Local);

    if (isGlobal) {
        globalPresets.push_back(preset);
        saveGlobalData();
    } else {
        localPresets.push_back(preset);
        saveLocalData();
    }
}

void AdjustmentManager::createPresetFromActiveCellIfMissing(bool isGlobal) {
    if (!activeCell) return;

    // Presets must reference linked data.
    if (!activeCell->isLinked()) {
        activeCell->id = generateId();
    }

    // Ensure the linked data is managed and persisted.
    activeCell->isGlobal = isGlobal;
    adjustmentDataMap[activeCell->id] = activeCell;

    auto hasPresetWithDataId = [this](const QUuid& id) {
        for (const auto& preset : localPresets) {
            if (preset && preset->data && preset->data->id == id) return true;
        }
        for (const auto& preset : globalPresets) {
            if (preset && preset->data && preset->data->id == id) return true;
        }
        return false;
    };

    if (hasPresetWithDataId(activeCell->id)) {
        return;
    }

    auto preset = std::make_shared<Preset>(
        activeCell,
        activeCell->name,
        isGlobal ? PresetScope::Global : PresetScope::Local
    );

    if (isGlobal) {
        globalPresets.push_back(preset);
        saveGlobalData();
    } else {
        localPresets.push_back(preset);
        saveLocalData();
    }
}

void AdjustmentManager::renamePreset(std::shared_ptr<Preset> preset, const QString& newName) {
    if (!preset || !preset->data) return;

    const QString trimmedName = newName.trimmed();
    if (trimmedName.isEmpty()) return;

    preset->name = trimmedName;
    preset->data->name = trimmedName;

    if (preset->scope == PresetScope::Global) {
        saveGlobalData();
    } else {
        saveLocalData();
    }

    // Notify UI subscribers that linked cell-data metadata changed.
    emit linkedCellDataChanged(preset->data->id);
}

void AdjustmentManager::removePreset(std::shared_ptr<Preset> preset) {
    if (!preset) return;

    if (preset->scope == PresetScope::Local) {
        localPresets.remove(preset);
        saveLocalData();
    } else {
        globalPresets.remove(preset);
        saveGlobalData();
    }

    // Note: We keep the cell data in cellDataMap to maintain links to existing cells
}

const std::list<std::shared_ptr<Preset>>& AdjustmentManager::getLocalPresets() const {
    return localPresets;
}

const std::list<std::shared_ptr<Preset>>& AdjustmentManager::getGlobalPresets() const {
    return globalPresets;
}

// Apply the active cell to all selected images based on the current apply mode.
void AdjustmentManager::apply() {
    if (!activeCell) return;

    if (applyMode == ApplyMode::Link) {
        // Ensure the active cell has an ID (needed for linking)
        if (!activeCell->isLinked()) {
            activeCell->id = generateId();
            adjustmentDataMap[activeCell->id] = activeCell;
            // Save to local file (not global, since this is from current image)
            updateCellData(activeCell->id);
            
            // Also update the source image's sidecar if available
            if (activeImage) {
                activeImage->saveAdjustmentCells();
            }
        }
    }

    for (const auto& image : selectedImages) {
        if (!image) continue;

        // Create a new cell instance for this image
        auto newCell = std::make_shared<AdjustmentCell>();

        if (applyMode == ApplyMode::Link) {
            // Create a linked cell
            newCell->adjustmentGroup = activeCell;
        } else {
            // Create a copy (static) cell with deep-copied adjustments
            newCell->adjustmentGroup = std::make_shared<AdjustmentGroup>(activeCell->name);
            newCell->adjustmentGroup->isEnabled = activeCell->isEnabled;

            // Deep copy adjustments
            newCell->adjustmentGroup->adjustments = activeCell->cloneAdjustments();
        }

        // Add cell to image
        image->adjustmentCells.push_back(*newCell);
        image->saveAdjustmentCells();
    }
}

void AdjustmentManager::changeCellVisibility(std::shared_ptr<AdjustmentGroup> cellData, bool visible) {
    if (cellData) {
        cellData->isEnabled = visible;
        updateCellData(cellData->id);
    }
}

// Detach the given cell from its linked data, making it a static cell with copied adjustments.
void AdjustmentManager::notifyCellDataChanged(std::shared_ptr<AdjustmentGroup> cellData) {
    if (cellData) {
        if (cellData->isLinked()) {
            // Persist the updated cell data to the local/global preset file
            updateCellData(cellData->id);
        } else {
            // For unlinked cells, save the active image
            for (const auto& image : selectedImages) {
                if (image) {
                    image->saveAdjustmentCells();
                }
            }
        }
        // Notify viewers to re-render (works for both linked and unlinked)
        emit linkedCellDataChanged(cellData->id);
    }
}

std::list<std::shared_ptr<Image>>::iterator AdjustmentManager::findImageInSelection(std::shared_ptr<Image> image) {
    return std::find(selectedImages.begin(), selectedImages.end(), image);
}

QUuid AdjustmentManager::generateId() {
    // Generate a UUID based on counter for deterministic testing
    // In production, could use QUuid::createUuid()
    return QUuid::createUuid();
}
