# Dynamic Adjustment Cells System - Implementation Summary

## Overview
A complete system for managing adjustment cells with dynamic linking capabilities has been implemented. This allows users to create adjustment presets that can be shared across multiple images, with changes propagating dynamically to all linked instances.

## Architecture

### Core Classes

#### 1. **AdjustmentCellData**
The shared data container that multiple cells can reference.

**Location**: `adjustmentcelldata.h/cpp`

**Key Members**:
- `QUuid id` - Unique identifier (null if not linked)
- `QString name` - Name of the cell data
- `bool enabled` - Visibility toggle
- `bool isGlobal` - Scope (local or global)
- `std::unordered_map<AdjType, std::unique_ptr<Adjustment>> adjustments` - Adjustment data

**Key Methods**:
- `bool isLinked()` - Check if this cell data is dynamically linked
- `static createDefault()` - Factory method for default cells

#### 2. **AdjustmentCell** (Refactored)
Instance of an adjustment cell that can optionally share data with other cells.

**Location**: `adjustmentcell.h/cpp`

**Key Members**:
- `std::shared_ptr<AdjustmentCellData> data` - Shared cell data
- `QString instanceName` - Name for this specific instance
- `bool visible` - Visibility of this cell instance
- `std::vector<AdjType> displayOrder` - UI display order
- `std::vector<AdjType> processOrder` - Rendering order

**Key Methods**:
- `AdjustmentCell(const QString& name)` - Create unlinked cell
- `AdjustmentCell(std::shared_ptr<AdjustmentCellData>, const QString& name)` - Create linked cell
- `bool isLinked()` - Check if cell is linked to shared data

#### 3. **AdjustmentCellManager**
Central manager for all adjustment cell operations.

**Location**: `adjustmentcellmanager.h/cpp`

**Key Members**:
- `std::unordered_map<QUuid, std::shared_ptr<AdjustmentCellData>> cellDataMap` - All linked cell data
- `std::list<std::shared_ptr<Preset>> localPresets` - Local presets
- `std::list<std::shared_ptr<Preset>> globalPresets` - Global presets
- `std::shared_ptr<AdjustmentCellData> activeCell` - Currently active cell
- `std::shared_ptr<Image> activeImage` - Currently active image
- `std::list<std::shared_ptr<Image>> selectedImages` - Multiple selected images
- `ApplyMode applyMode` - Copy or Link mode
- `PresetScope presetListMode` - Local or Global mode

**Key Methods**:
- `initialize(const QString& localGroupPath)` - Initialize with paths
- `loadData(const QString& localGroupPath)` - Load from files
- `saveData()` - Save to files
- `setActiveCell/Image()` - Set active selections
- `selectImage()`, `addToSelection()`, `addRangeToSelection()` - Image selection
- `setApplyMode()`, `setPresetListMode()` - Mode configuration
- `createPreset()` - Save cell as preset
- `removePreset()` - Remove preset
- `apply()` - Apply active cell to selected images
- `changeCellVisibility()` - Toggle cell visibility

#### 4. **Preset**
Represents a saved preset reference.

**Location**: `adjustmentcellmanager.h`

**Structure**:
```cpp
struct Preset {
    QUuid dataId;
    QString name;
    PresetScope scope;  // Local or Global
};
```

### UI Components

#### 1. **AdjustmentCellManagerWidget**
Main UI for preset and cell management.

**Location**: `ui/adjustmentcellmanagerwidget.h/cpp`

**Features**:
- Local/Global preset mode toggle button
- Scrollable preset list with remove buttons
- Active cell context menu:
  - Eye button to toggle visibility
  - Apply button with Copy/Link mode toggle
  - Save as Preset button
  - Shows active cell name

**Connections**:
- Receives updates from AdjustmentCellManager
- Updates preset list when mode changes
- Applies cells to selected images

#### 2. **AdjustmentPanelWidget** (Updated)
Displays adjustment cells for the current image.

**Location**: `ui/adjustmentpanelwidget.h/cpp`

**New Features**:
- "+ Create Cell" button at the top
- Creates new unlinked (static) adjustment cells
- Dynamically updates when cells are created/removed

### Image Class (Updated)

**Location**: `image.h/cpp`

**New Methods**:
- `loadAdjustmentCells(AdjustmentCellManager* acm)` - Load cells from sidecar file and linked data
- `saveAdjustmentCells()` - Save cells to sidecar JSON file
- `QString getSidecarPath()` - Get path to sidecar file

**Sidecar File Location**: `imagename.adjustments.json` (in same directory as raw file)

### MainWindow (Updated)

**Location**: `ui/mainwindow.h/cpp`

**Changes**:
- Added `AdjustmentCellManager` instance
- Added `AdjustmentCellManagerWidget` to UI
- Connected gallery selection to update active image in manager
- Reorganized layout to include ACM widget

## Data Storage

### Sidecar Files (.adjustments.json)
Located next to each raw image file.

```json
{
  "imagePath": "photo.ARW",
  "cells": [
    {
      "visible": true,
      "linked": true,
      "dataId": "550e8400-e29b-41d4-a716-446655440000",
      "name": "Default",
      "adjustments": {
        "exposure": 0.3,
        "contrast": 1.1
      }
    }
  ]
}
```

### Local Preset File (.adjustment_cells.json)
Located in the image directory.

```json
{
  "cellData": [
    {
      "id": "550e8400-e29b-41d4-a716-446655440000",
      "name": "Outdoor",
      "isEnabled": true,
      "adjustments": {
        "exposure": 0.5,
        "contrast": 1.2,
        "saturation": 1.1
      }
    }
  ],
  "presets": [
    {
      "dataId": "550e8400-e29b-41d4-a716-446655440000",
      "name": "Outdoor"
    }
  ]
}
```

### Global Preset File (global_presets.json)
Located in `~/.raw-processor/global_presets.json`.

Same structure as local presets but available across all directories.

## User Workflows

### Creating a Static Cell
1. Click "+ Create Cell" in AdjustmentPanelWidget
2. Adjust settings in the new cell
3. Cell is stored in image's sidecar file

### Creating a Preset from Active Cell
1. Select or create adjustment cell
2. In AdjustmentCellManagerWidget, click "Save as Preset"
3. Choose Local or Global scope
4. Cell data is saved to preset file

### Applying Cell to Selected Images
1. Make active image and select it
2. Select multiple images (Ctrl+Click, Shift+Click in gallery)
3. Select or create adjustment cell to apply
4. Choose Copy (static) or Link (dynamic) mode
5. Click "Apply"
6. New cells are added to selected images

### Linking Cells Dynamically
1. Create adjustment cell and save as preset
2. Apply to other images with "Link" mode
3. Changes to the source cell propagate to all linked cells automatically
4. Each image stores a reference (ID) to the linked cell data

## Integration Points

### Gallery Widget
When an image is selected, it notifies MainWindow which:
- Sets the image in ImageViewer
- Sets the image in AdjustmentPanelWidget
- Sets the active image in AdjustmentCellManager

### Image Loading
When an image is loaded:
- `loadRawData()` loads raw pixel data
- `loadAdjustmentCells()` loads adjustment cells from sidecar file
- Linked cells retrieve data from AdjustmentCellManager

### Image Saving
When adjustment cells are modified:
- `saveAdjustmentCells()` writes to sidecar file
- If linked, reference is stored (ID)
- If cell data modified, manager updates preset files

## Key Features

### 1. Dynamic Linking
Multiple cells can reference the same AdjustmentCellData:
```cpp
auto cellData = std::make_shared<AdjustmentCellData>();
AdjustmentCell cell1(cellData);  // Links to cellData
AdjustmentCell cell2(cellData);  // Also links to same cellData
// Changes to cellData affect both cells
```

### 2. Local vs Global Presets
- **Local**: Only accessible when editing images in that directory
- **Global**: Accessible from any directory
- Separate storage files for organization

### 3. Copy vs Link Modes
- **Copy**: Static copy of adjustments - future changes to source don't affect
- **Link**: Dynamic reference - future changes to source propagate

### 4. Multi-Image Selection
- Click: Select single image
- Ctrl+Click: Add/remove from selection
- Shift+Click: Range selection
- Presets can be applied to all selected images at once

### 5. Cell Visibility
- Eye button toggles visibility per cell instance
- Visibility stored in sidecar file
- Can affect rendering pipeline (needs integration)

## Implementation Notes

### What's Complete
✅ Core data structures (AdjustmentCellData, Preset)
✅ AdjustmentCell refactoring to support dynamic linking
✅ AdjustmentCellManager with all core functionality
✅ File I/O framework for sidecar and preset files
✅ UI widgets (AdjustmentCellManagerWidget)
✅ Image class integration for persistence
✅ MainWindow integration

### What Still Needs Work
⚠️ **Adjustment Serialization**: JSON parsing for individual adjustment types
   - Currently has `// TODO: Parse adjustments from JSON` placeholders
   - Need to implement serialization for each AdjType

⚠️ **Deep Copying**: Proper copying of adjustment instances
   - Currently uses placeholder `std::make_unique<AdjExposure>()`
   - Need to implement clone/copy methods for Adjustment classes

⚠️ **Rendering Integration**: 
   - Cell visibility doesn't affect rendering yet
   - Need to integrate with ImageProcessor to respect cell visibility
   - Need to apply cells in correct process order

⚠️ **UI Polish**:
   - Buttons could use icons instead of text
   - Could add dialogs for naming presets
   - Could improve visual layout

⚠️ **Error Handling**:
   - File I/O could have more robust error handling
   - Missing adjustments should be handled gracefully
   - Invalid UUIDs should be detected

### Future Enhancements
1. **Undo/Redo** for cell operations
2. **Cell History** showing recent presets
3. **Preset Categories** for organization
4. **Cell Comparison** side-by-side view
5. **Batch Operations** apply to all images in directory
6. **Export/Import** presets between projects

## Testing Recommendations

1. **Unit Tests**:
   - AdjustmentCellManager ID generation
   - Preset creation/removal
   - File I/O paths

2. **Integration Tests**:
   - Load image → create cell → save → reload
   - Create preset → apply to multiple images
   - Update linked cell → verify propagation

3. **UI Tests**:
   - Gallery selection updates ACM
   - Preset list updates on mode toggle
   - Create cell button works

4. **Data Persistence**:
   - Verify sidecar files created correctly
   - Verify presets persist across sessions
   - Verify linked cells maintain references

## Files Created/Modified

### New Files
- `adjustmentcelldata.h/cpp` - AdjustmentCellData class
- `adjustmentcellmanager.h/cpp` - AdjustmentCellManager class
- `ui/adjustmentcellmanagerwidget.h/cpp` - ACM UI widget

### Modified Files
- `adjustmentcell.h/cpp` - Refactored to use AdjustmentCellData
- `image.h/cpp` - Added sidecar file I/O
- `ui/adjustmentpanelwidget.h/cpp` - Added Create Cell button
- `ui/mainwindow.h/cpp` - Integrated ACM
- `CMakeLists.txt` - Added new files to build

### Updated Includes
- All necessary Qt includes added (QJsonDocument, QUuid, etc.)
- Forward declarations used to avoid circular dependencies
