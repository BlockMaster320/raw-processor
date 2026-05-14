#pragma once

#include "adjustmentgroup.h"

#include <memory>
#include <list>
#include <map>

#include <QString>
#include <QUuid>
#include <QObject>

// Custom hash function for QUuid
// #include <QUuid>
// #include <QHashFunctions>

// struct QUuidHasher {
//     std::size_t operator()(const QUuid& id) const noexcept {
//         return static_cast<std::size_t>(qHash(id));
//     }
// };

class Image;

// Scope of a preset (Local or Global)
enum class PresetScope {
    Local,
    Global
};

// Represents a saved preset - a reference to shared adjustment cell data
struct Preset {
    std::shared_ptr<AdjustmentGroup> data;
    QString name;
    PresetScope scope;

    Preset(std::shared_ptr<AdjustmentGroup> cellData, const QString& presetName, PresetScope presetScope)
        : data(std::move(cellData)), name(presetName), scope(presetScope) {}
};

// Central manager for adjustment cells, presets, and dynamic linking
// Handles:
// - Storage and retrieval of linked cell data
// - Local and global presets
// - Applying cells to images
// - Saving/loading from JSON files
class AdjustmentManager : public QObject {
    Q_OBJECT
public:
    AdjustmentManager();
    ~AdjustmentManager();

    // Initialize manager with paths and load existing data
    void initialize(const QString& localGroupPath);

    // File I/O
    void loadData(const QString& localGroupPath);
    void saveData();
    void updateCellData(const QUuid& id);

    // Cell data retrieval
    std::shared_ptr<AdjustmentGroup> getCellData(const QUuid& id) const;

    // Register a cell data that was loaded from a sidecar but not in the manager
    void registerCellData(std::shared_ptr<AdjustmentGroup> cellData);

    // Refresh a linked cell's data from the file (ensures latest values)
    void refreshCellData(const QUuid& id);

    // Active cell and image management
    void setActiveCell(std::shared_ptr<AdjustmentGroup> cellData);
    std::shared_ptr<AdjustmentGroup> getActiveCell() const;

    void setActiveImage(std::shared_ptr<Image> image);
    std::shared_ptr<Image> getActiveImage() const;

    void selectImage(std::shared_ptr<Image> image, bool clearSelection = false);
    void addToSelection(std::shared_ptr<Image> image);
    void addRangeToSelection(std::shared_ptr<Image> fromImage, std::shared_ptr<Image> toImage);
    const std::list<std::shared_ptr<Image>>& getSelectedImages() const;
    void clearSelection();

    // Apply modes
    enum class ApplyMode { Copy, Link };
    void setApplyMode(ApplyMode mode);
    ApplyMode getApplyMode() const;

    // Preset modes
    void setPresetListMode(PresetScope scope);
    PresetScope getPresetListMode() const;

    // Preset operations
    void createPreset(const QString& presetName, bool isGlobal);
    void createPresetFromActiveCellIfMissing(bool isGlobal);
    void renamePreset(std::shared_ptr<Preset> preset, const QString& newName);
    void removePreset(std::shared_ptr<Preset> preset);
    const std::list<std::shared_ptr<Preset>>& getLocalPresets() const;
    const std::list<std::shared_ptr<Preset>>& getGlobalPresets() const;

    // Apply active cell to selected images
    void apply();

    // Cell visibility
    void changeCellVisibility(std::shared_ptr<AdjustmentGroup> cellData, bool visible);

    // Notify that a linked cell's data has been modified (e.g., slider moved)
    void notifyCellDataChanged(std::shared_ptr<AdjustmentGroup> cellData);

signals:
    void linkedCellDataChanged(QUuid cellDataId);

private:
    // File paths
    QString localFilePath;
    QString globalFilePath;

    // Cell data storage (linked cells)
    std::map<QUuid, std::shared_ptr<AdjustmentGroup>> adjustmentDataMap;

    // Presets
    std::list<std::shared_ptr<Preset>> localPresets;
    std::list<std::shared_ptr<Preset>> globalPresets;

    // Active selections
    std::shared_ptr<AdjustmentGroup> activeCell;
    std::shared_ptr<Image> activeImage;
    std::list<std::shared_ptr<Image>> selectedImages;

    // Modes and settings
    ApplyMode applyMode = ApplyMode::Copy;
    PresetScope presetListMode = PresetScope::Local;

    // ID counter for generating unique IDs
    uint32_t idCounter = 0;

    // File I/O helpers
    void loadLocalData();
    void loadGlobalData();
    void saveLocalData();
    void saveGlobalData();

    // Generate unique ID
    QUuid generateId();

    // Helper to find image in selection
    std::list<std::shared_ptr<Image>>::iterator findImageInSelection(std::shared_ptr<Image> image);
};
