User Manual
===========================

Overview
--------
raw-processor is a desktop RAW photo editor focused on non-destructive adjustments and fast batch work.

Main capabilities:
- Load a folder of RAW photos (.ARW, .CR2, .NEF, .DNG) into a thumbnail gallery.
- Open any image in the main viewer and edit it using adjustment cells.
- Save adjustments non-destructively to sidecar files next to each image.
- Create reusable presets (Local or Global).
- Apply the active cell to multiple selected images using Copy or Link mode.
- Export selected images to JPEG.

Application layout
------------------
The window has four main areas:
1) Left panel (file and preset/cell management)
2) Image viewer (displays the resulting image)
3) Gallery (thumbnails of currently loaded images)
4) Right panel (adjustment cells and sliders for the active image)

Quick start workflow
--------------------
1. Click "Load images" and choose a folder.
3. Double-click a thumbnail in the gallery to open the image in the viewer and load its adjustment cells in the right panel.
4. A "Base" adjustment cell will be created by default for a newly loaded image. Create new adjustment cells by clicking the "Add adjustment cell" button.

5. Optionally apply the active cell to other selected images (Copy or Link).
6. Export selected images with "Export selected images".

All interactive elements
------------------------

Left panel
----------
Load images button:
- Opens a folder picker.
- Loads supported RAW files from that folder into the gallery.

Export selected images button:
- Enabled only when at least one image is selected in the gallery.
- Opens an output-folder picker.
- Exports selected images as JPEG (.jpg).
- If a target file name already exists, a numeric suffix is appended automatically.

Presets section:
- Preset list: shows presets for the currently selected scope (Local/Global).
- Double-click preset name (or edit key) to rename.
- Single click a preset to activate it (makes that preset's adjustment group active).
- Local/Global switch:
	- Local = presets saved for the currently loaded image folder.
	- Global = presets shared across folders (stored in user home profile data).
- Remove Selected Preset: removes the preset entry from the selected scope.

Active cell/preset section:
- Shown when an active cell exists.
- Displays the active cell/preset name.
- Enabled/Disabled switch:
	- Enabled: cell contributes to processing.
	- Disabled: cell remains present but is skipped (dimmed in UI).
- Save as Preset:
	- Creates a new preset from the currently active cell (scope depends on Local/Global switch).

Adjustment application panel:
- Apply button: applies active cell to all currently selected gallery images.
- Copy/Link switch:
	- Copy: creates independent copies of the cell (future edits do not sync).
	- Link: creates linked instances that share one adjustment group (future edits sync).
- Add linked copy as preset switch:
	- Only active in Link mode.
	- When ON, applying in Link mode also auto-creates a preset if missing.

Center area
-----------
Gallery (top):
- Mouse wheel: scroll thumbnails horizontally.
- Single left click:
	- No modifier: select one image.
	- Ctrl: add image to selection.
	- Shift: select range from last selected thumbnail.
- Double left click: open image in the viewer and load its adjustment cells.

Image viewer (bottom):
- Left mouse drag: pan image.
- Mouse wheel: zoom in/out.
- Keyboard compare shortcuts:
	- 0: normal processed view
	- 1: split compare (processed vs reference)
	- 2: absolute difference view
	- 3: signed difference view

Right panel (Adjustment cells)
------------------------------
Add Adjustment Cell button:
- Adds a new, unlinked cell to the active image.

Each adjustment cell header has:
- Name field:
	- Double-click to rename.
	- For linked cells, rename propagates to linked instances and matching preset names.
- Collapse/expand button: show/hide slider controls.
- Eye button: toggle cell visibility (enabled/disabled contribution).
- Unlink button:
	- Available only for linked cells.
	- Detaches this cell into an independent copy.
- Trash button: remove this cell from the image.

Cell body:
- Sliders grouped by sections (for example White balance, Tone, Color, Effects, Detail).
- Dragging sliders updates rendering.

Cell ordering:
- Drag a cell by mouse to reorder it in the panel.
- Order affects processing sequence.

Important concepts
------------------

What "active cell/preset" means
--------------------------------
The active cell/preset is the currently focused adjustment group used by management actions.

It is set when you:
- Click/activate an adjustment cell in the right panel, or
- Select a preset in the presets list.

The active cell/preset is what:
- Enabled/Disabled controls modify,
- "Save as Preset" uses as source,
- "Apply" uses when applying to selected images.

If no cell is active, active-cell controls are hidden/disabled.

What linked/shared adjustment groups mean
-----------------------------------------
A linked cell points to shared adjustment data (one shared group ID).

Effect:
- Editing one linked instance updates all other instances sharing that same group.
- Presets can reference the same shared group.

Unlinked (copy) cells have their own private adjustment data.

Blue circle indicator in gallery
--------------------------------
A small blue dot on a gallery thumbnail means:
- That image contains at least one linked cell that shares the same adjustment group as the current active linked cell/preset.

In practice, it highlights images that are part of the same shared adjustment group context.

Gallery controls (image selection)
----------------------------------
Selection rules:
- Single click = single selection.
- Ctrl + click = additive multi-selection.
- Shift + click = range selection from last clicked image.
- Clicking another image without Ctrl/Shift replaces selection.

Using selection:
- Selection controls batch actions:
	- Apply active cell to selected images.
	- Export selected images.

Persistence and files
---------------------
Per-image sidecar files:
- Saved next to each RAW image as:
	<image_base_name>.adjustments.json

Preset storage:
- Local presets/cells metadata: in selected image folder as:
	.adjustment_cells.json
- Global presets/cells metadata: in user home profile as:
	~/.raw-processor/global_presets.json

Notes and behavior details
--------------------------
- If an image has no sidecar yet, a default "Base" cell is created.
- Export always writes JPEG output, quality 95.
- Remove preset deletes the preset entry, not existing cells already using that data.
- Linked changes trigger synchronized updates across linked instances.
