# Dynamic Adjustment Cells - Complete Implementation

## Summary of Implemented Features

All previously missing features have been successfully implemented:

### 1. ✅ Adjustment Serialization

**Files Updated**: `adjustment.h`, `adjustment.cpp`, `adjustmentcelldata.h/cpp`

**Implementation**:
- Added `clone()` virtual method to `Adjustment` base class
- Implemented `clone()` for all adjustment types (Exposure, Contrast, Midpoint, PopArt, WhiteBlack, Saturation, Denoise)
- Clone creates a deep copy with all attributes preserved
- Added `toJson()` and `fromJson()` methods to `AdjustmentCellData`
- JSON serialization includes:
  - Cell ID, name, enabled state, global flag
  - All adjustments with their attribute values (min, max, current value, adjustability)
  - Nested JSON structure organized by adjustment type

**Key Methods**:
```cpp
class AdjustmentCellData {
    std::unordered_map<AdjType, std::unique_ptr<Adjustment>> cloneAdjustments() const;
    QJsonObject toJson() const;
    void fromJson(const QJsonObject& obj);
};

class Adjustment {
    virtual std::unique_ptr<Adjustment> clone() const = 0;
};
```

### 2. ✅ Deep Copying

**Files Updated**: `adjustment.cpp`, `adjustmentcelldata.cpp`, `adjustmentcellmanager.cpp`

**Implementation**:
- Each adjustment type has a `clone()` method that creates a new instance with copied attributes
- `AdjustmentCellData::cloneAdjustments()` performs deep copy of entire adjustment map
- When applying in "Copy" mode, uses deep copies instead of references
- When creating presets, adjustments are cloned to ensure independence

**Applied in These Operations**:
1. `AdjustmentCellManager::apply()` - Deep copies adjustments when in Copy mode
2. `AdjustmentCellManager::createPreset()` - Deep copies when saving as preset
3. `Image::loadAdjustmentCells()` - Creates fresh adjustments when loading cached data

### 3. ✅ Rendering Integration (Visibility)

**Files Updated**: `imageprocessor.cpp`, `adjustmentcell.h`

**Implementation**:
- Updated `renderAdjustmentPass()` to check both instance visibility and data enabled state
- Cell is only applied if:
  - `cell.visible == true` (instance visibility toggle)
  - `cell.data != nullptr` (data exists)
  - `cell.data->enabled == true` (cell data is enabled)
- Process order respected - adjustments applied in correct rendering order
- Visibility changes immediately affect rendering

**Code Example**:
```cpp
for (auto& cell : currentImage->adjustmentCells) {
    if (!cell.visible || !cell.data || !cell.data->enabled)
        continue;  // Skip this cell
    
    // Apply adjustments in order
    for (AdjType type : cell.processOrder) {
        auto it = cell.data->adjustments.find(type);
        if (it != cell.data->adjustments.end())
            it->second->apply(*this);
    }
}
```

### 4. ✅ File I/O Updates

**Files Updated**: `adjustmentcellmanager.cpp`, `image.cpp`

**Sidecar File Loading/Saving**:
```json
{
  "imagePath": "photo.ARW",
  "cells": [
    {
      "visible": true,
      "linked": true,
      "dataId": "550e8400-e29b-41d4-a716-446655440000",
      "name": "Portrait",
      "adjustments": {
        "Exposure": {
          "attributes": [
            {
              "name": "Exposure",
              "value": 0.5,
              "min": -5.0,
              "max": 5.0,
              "adjustable": true
            }
          ]
        }
      }
    }
  ]
}
```

**Preset File Loading/Saving**:
- Local presets: `.adjustment_cells.json` in image directory
- Global presets: `~/.raw-processor/global_presets.json`
- Both files use same structure with cellData array and presets array

**Updated Methods**:
- `AdjustmentCellManager::loadLocalData()` - Uses `fromJson()`
- `AdjustmentCellManager::loadGlobalData()` - Uses `fromJson()`
- `AdjustmentCellManager::saveLocalData()` - Uses `toJson()`
- `AdjustmentCellManager::saveGlobalData()` - Uses `toJson()`
- `Image::loadAdjustmentCells()` - Loads and deserializes with fallback to defaults
- `Image::saveAdjustmentCells()` - Serializes all cells to sidecar

## Complete Feature Checklist

### Core System
- ✅ Dynamic linking between cells
- ✅ Local and global presets
- ✅ Copy and Link apply modes
- ✅ Multi-image selection
- ✅ Cell visibility control

### Data Persistence
- ✅ Sidecar file format (.adjustments.json)
- ✅ Local preset storage and loading
- ✅ Global preset storage and loading
- ✅ JSON serialization for all adjustment types
- ✅ Cached adjustment data for broken links

### Rendering
- ✅ Cell visibility affects rendering
- ✅ Correct adjustment application order
- ✅ Both instance and data visibility checks
- ✅ Efficient update on visibility toggle

### UI Components
- ✅ AdjustmentCellManagerWidget for preset management
- ✅ Create Cell button in AdjustmentPanelWidget
- ✅ Mode toggles (Local/Global, Copy/Link)
- ✅ Preset list with remove functionality
- ✅ Eye button for visibility toggle

## Testing Scenarios

### Scenario 1: Create and Save Preset
1. Adjust settings on active cell
2. Click "Save as Preset" with Global scope
3. Preset is saved to global file with all adjustments serialized
4. Adjustments values are preserved across sessions

### Scenario 2: Apply with Link Mode
1. Create preset and apply to multiple images with Link mode
2. Modify source cell adjustments
3. All linked cells reflect changes
4. Changes persist through save/load cycle

### Scenario 3: Apply with Copy Mode
1. Create preset and apply to multiple images with Copy mode
2. Modify source cell adjustments
3. Copies remain unchanged (independent)
4. Each image has its own adjustment copy in sidecar

### Scenario 4: Visibility Toggling
1. Create and apply cells to image
2. Toggle visibility using eye button
3. Invisible cells are skipped during rendering
4. Visibility state persists in sidecar file

### Scenario 5: Load Image with Linked Cells
1. Image has cells linked to presets that no longer exist
2. System loads cached adjustment values as fallback
3. Cached values are used if source data unavailable
4. User sees a complete set of adjustments

## Implementation Details

### Adjustment Cloning Strategy
Each adjustment class implements clone by:
1. Creating new instance of same type
2. Copying all attributes vector (values, min, max, adjustability)
3. Returning as unique_ptr<Adjustment>

### JSON Serialization Strategy
- Adjustment types mapped to string names (Exposure, Contrast, etc.)
- Each adjustment stores array of attributes
- Full state restoration when deserializing
- Graceful handling of missing adjustments

### Visibility Implementation
Two-level visibility system:
1. **Instance Level** (`cell.visible`) - Controls whether this specific cell is applied
2. **Data Level** (`cell.data->enabled`) - Controls whether the shared data is enabled

Both must be true for adjustments to apply.

### File Organization
```
image_directory/
  photo.ARW
  photo.ARW.adjustments.json        # Instance data per image
  .adjustment_cells.json             # Local presets

~/.raw-processor/
  global_presets.json               # Global presets
```

## Performance Considerations

- **Memory**: Deep copies only created when needed (Copy mode, preset creation)
- **Disk I/O**: JSON files only written when data changes
- **Rendering**: Visibility checks are O(1) per cell, minimal overhead
- **Serialization**: JSON parsing done once at load, not in render loop

## Error Handling

- Missing source data in linked cells handled with cached values
- Invalid JSON gracefully ignored, defaults used
- Missing adjustment types initialized with defaults
- File write failures logged but don't crash application

## Future Enhancement Opportunities

1. **Undo/Redo**: Track changes to adjustment values
2. **Cell Blending**: Layer multiple cells with blend modes
3. **Presets Library**: UI to browse and organize presets
4. **Export**: Save presets as shareable files
5. **Versioning**: Track preset modification history
6. **Comparison**: Side-by-side cell comparison view

## Compatibility Notes

- **JSON Format**: Human-readable, compatible with manual editing
- **Qt Versions**: Works with Qt6 (uses QJsonDocument)
- **Cross-Platform**: File paths use QDir for platform independence
- **UUID Format**: Standard QUuid format for cell IDs
