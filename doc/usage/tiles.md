# Tile Extraction

<div align="center"><img src="../img/tiles-build-item.png" alt="Tile extraction editor" width="800" /></div>

A **Tiles** build item converts a source image for a target system and then slices the converted result into a regular grid of tiles. Each tile can be individually previewed and selectively excluded from the final output.

## Creating a tileset conversion

To create a tileset conversion, right-click an image file in the **Files** panel and select **Add tileset conversion**. The image must already be included in the project. The new build item takes its name from the image filename (without extension) and appears in the **Build** section of the Project panel. Double-click it to open the tileset document.

To remove a tileset conversion, right-click its entry in the Build section and select **Remove tileset conversion**.

## Document layout

The tileset document has two tabs: **Conversion** and **Tile Extraction**.

## Conversion tab

The Conversion tab is identical to the Bitmap document. It shows the source image and the converted preview side by side, with the palette and conversion parameters in the right-hand tooling panel.

See [bitmaps.md](bitmaps.md) for a full description of the conversion workflow, palette controls, and preview options.

## Tile Extraction tab

The Tile Extraction tab is where tiles are defined, extracted, and managed.

### Layout

The tab is divided into three areas:

- **Left top — dual image viewer.** The left panel shows the full converted preview with a selection rectangle highlighting the currently selected tile. The right panel shows the selected tile in isolation. Both panels have independent zoom and pan controls; they are not synchronised.
- **Left bottom — tile list.** A scrollable grid of all tile thumbnails.
- **Right — tooling panel.** Contains the tile extraction parameters and the export widget.

### Preview controls

Each panel in the dual viewer has its own **Aspect correction** and **Scanlines** checkboxes. On the left panel, toggling these options re-runs the conversion. On the right panel (single tile view), they regenerate the tile preview using `GeneratePreview` without re-running the full conversion.

For zoom, pan, and pixel grid behaviour, the controls are the same as described in the Preview section of [bitmaps.md](bitmaps.md).

### Tile extraction parameters

The **Tile Extraction** collapsible section in the right tooling panel contains all grid parameters.

**Tile Size**

| Parameter | Description |
|---|---|
| Width | Width of each tile in pixels. Minimum value is 1. |
| Height | Height of each tile in pixels. Minimum value is 1. |

**Starting Offset**

| Parameter | Description |
|---|---|
| Offset X | Horizontal offset in pixels from the left edge of the converted image before the grid starts. Minimum value is 0. |
| Offset Y | Vertical offset in pixels from the top edge of the converted image before the grid starts. Minimum value is 0. |

**Grid Padding**

| Parameter | Description |
|---|---|
| Padding X | Horizontal gap in pixels between adjacent tiles in the grid. Minimum value is 0. |
| Padding Y | Vertical gap in pixels between adjacent tiles in the grid. Minimum value is 0. |

Below the parameters, the panel displays:

- **Tiles Extracted** — the number of tiles currently extracted.
- **Grid Size** — the computed grid dimensions as columns × rows, derived from the converted image size and the parameters above.

Any change to any grid parameter immediately clears the deleted tiles list, since the grid structure has changed and previous deletion indices are no longer valid.

Tile extraction always operates on the native-resolution converted image, not on the scaled preview.

### Extract Tiles button

Clicking **Extract Tiles** re-runs the full conversion and then slices the converted image into tiles according to the current parameters. The tile list updates immediately to show the new results. Extraction must be triggered manually after adjusting parameters.

### Tile list

The tile list shows all tile positions in the grid as 64×64 pixel thumbnails arranged in rows. The number of columns adjusts automatically to the available panel width.

Clicking a tile selects it. The selected tile appears in the right panel of the dual viewer, and a selection rectangle is drawn over its position in the left (full preview) panel.

A tooltip appears on hover showing the tile index and its pixel dimensions. For deleted tiles the tooltip shows **DELETED** instead of the dimensions.

**Deleting and restoring tiles**

Right-clicking a tile opens a context menu with a single action: **Delete** or **Undelete**, depending on the current state of that tile. Deleted tiles remain visible in the list but are rendered as a dark red placeholder with a red cross drawn over them. Their index label is shown in red. Deleted tiles are excluded from export.

## Using tiles in a map

Once extracted, the tile sheet is available as a tileset source in the **Map** editor.

See [maps.md](maps.md) for details on painting maps with extracted tiles.

## Export

The tile extractor exposes all tile images and the deleted-tiles list to export scripts. See [export-scripts.md](export-scripts.md) for the `ITilesetContext` API.
