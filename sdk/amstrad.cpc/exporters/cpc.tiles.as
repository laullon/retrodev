// ---------------------------------------------------------------------------
//
// Retrodev SDK
//
// CPC tiles export script -- exports extracted tileset as raw byte data.
//
// (c) TLOTB 2026
//
// ---------------------------------------------------------------------------
//
// Script metadata
//
// @description Exports CPC tileset as raw byte data, one tile after another.
// @exporter tiles
// @target amstrad.cpc
// @param layout  combo  chunky  chunky  Byte layout per tile
// @param header  bool   false                        Prepend tile count byte
//
// ---------------------------------------------------------------------------
//
// Each tile is exported as a contiguous block of bytes.
// Byte order follows the CPC hardware encoding for the active video mode:
//
//   Mode 0 -- 2 pixels per byte, 16 colours
//   Mode 1 -- 4 pixels per byte,  4 colours
//   Mode 2 -- 8 pixels per byte,  2 colours
//
// "chunky" emits rows top-to-bottom (standard for software blitters).
//
// ---------------------------------------------------------------------------

#include "cpc.utils.as"

// ---------------------------------------------------------------------------
//
// Export entry point.
//
// void Export(const string& in outputPath, TilesetExportContext@ ctx)
//
// ---------------------------------------------------------------------------

void Export(const string& in outputPath, TilesetExportContext@ ctx)
{
    int tileCount = ctx.GetTileCount();
    int tileW     = ctx.GetTileWidth();
    int tileH     = ctx.GetTileHeight();
    string mode   = ctx.GetTargetMode();
    string system = ctx.GetTargetSystem();
    Palette@ palette = ctx.GetPalette();
    string layout        = ctx.GetParam("layout");
    bool writeHeader     = ctx.GetParam("header") == "true";
    //
    // Validate that we have tiles to export
    //
    if (tileCount <= 0) {
        Log_Error("Export failed: no tiles extracted -- run extraction first.");
        return;
    }
    //
    // Validate tile dimensions for the active mode
    //
    int ppb = 2;
    if (mode == "Mode 1") ppb = 4;
    else if (mode == "Mode 2") ppb = 8;
    if ((tileW % ppb) != 0) {
        Log_Error("Export failed: tile width " + tileW + " is not a multiple of "
            + ppb + " for " + mode + ".");
        return;
    }
    Log_Info("Export started -- system: " + system + "  mode: " + mode
        + "  tiles: " + tileCount + "  size: " + tileW + "x" + tileH
        + "  layout: " + layout
        + "  header: " + writeHeader
        + "  output: " + outputPath);
    //
    // Build the per-tile row order for the chosen layout
    //
    array<int> rowOrder(tileH);
    for (int r = 0; r < tileH; r++)
        rowOrder[r] = r;
    //
    // Encode all tiles into a single byte buffer
    //
    array<uint8> buf;
    int bytesPerRow = tileW / ppb;
    buf.reserve(tileCount * tileH * bytesPerRow + (writeHeader ? 1 : 0));
    //
    // Transparency settings used during pen resolution
    //
    bool useTransp = ctx.GetUseTransparentColor();
    int transpPen = ctx.GetTransparentPen();
    bool warnedNoTranspPen = false;
    bool warnedNoPen = false;
    //
    // Optional one-byte header: number of tiles (clamped to 255)
    //
    if (writeHeader)
        buf.insertLast(uint8(tileCount > 255 ? 255 : tileCount));
    for (int t = 0; t < tileCount; t++) {
        Image@ tile = ctx.GetTile(t);
        if (tile is null) {
            Log_Warning("Tile #" + t + " is null -- filling with zeros.");
            for (int b = 0; b < tileH * bytesPerRow; b++)
                buf.insertLast(uint8(0));
            continue;
        }
        //
        // Resolve pen indices for every pixel of this tile up front
        //
        array<int> penMap(tileW * tileH);
        for (int y = 0; y < tileH; y++) {
            for (int x = 0; x < tileW; x++) {
                RgbColor c = tile.GetPixelColor(x, y);
                //
                // Transparent pixel: alpha==0 (IsTransparent) or chroma-key enabled
                //
                bool isTransp = useTransp && c.IsTransparent();
                int pen;
                if (isTransp) {
                    pen = transpPen;
                    if (pen < 0) {
                        if (!warnedNoTranspPen) {
                            Log_Warning("Transparent pixels found but no transparent pen configured -- using pen 0.");
                            warnedNoTranspPen = true;
                        }
                        pen = 0;
                    }
                } else {
                    pen = palette.PenGetIndex(c);
                    if (pen < 0) {
                        if (!warnedNoPen) {
                            Log_Warning("One or more pixels have no matching pen -- using pen 0.");
                            warnedNoPen = true;
                        }
                        pen = 0;
                    }
                }
                penMap[y * tileW + x] = pen;
            }
        }
                //
                // Encode rows in the selected order and append to the output buffer
        //
        for (int i = 0; i < tileH; i++) {
            int y = rowOrder[i];
            EncodePixels(penMap, y * tileW, tileW, mode, buf);
        }
    }
    //
    // Write the buffer to disk
    //
    file@ f = file();
    if (f.open(outputPath, "w") < 0) {
        Log_Error("Export failed: could not open output file: " + outputPath);
        return;
    }
    for (uint i = 0; i < buf.length(); i++)
        f.writeUInt(buf[i], 1);
    f.close();
    Log_Info("Export finished -- " + tileCount + " tiles, "
        + buf.length() + " bytes written.");
}
