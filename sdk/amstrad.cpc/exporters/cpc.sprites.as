// ---------------------------------------------------------------------------
//
// Retrodev SDK
//
// CPC sprites export script -- exports extracted sprites as raw byte data.
//
// (c) TLOTB 2026
//
// ---------------------------------------------------------------------------
//
// Script metadata
//
// @description Exports CPC sprites as raw byte data, one sprite after another.
// @exporter sprites
// @target amstrad.cpc
// @param header  bool  false  Prepend sprite count byte
//
// ---------------------------------------------------------------------------
//
// Each sprite is exported as a contiguous block of bytes.
// Sprites can have arbitrary sizes; each row is encoded according to the
// active CPC video mode pixel-per-byte count:
//
//   Mode 0 -- 2 pixels per byte, 16 colours
//   Mode 1 -- 4 pixels per byte,  4 colours
//   Mode 2 -- 8 pixels per byte,  2 colours
//
// Rows are emitted top-to-bottom.  Sprites with a width that is not a
// multiple of the mode's pixels-per-byte count are rejected with an error.
//
// An optional one-byte header holds the total sprite count (clamped to 255).
// Each sprite block is preceded by a two-byte (width, height) size record
// so the loader can advance past variable-size sprites.
//
// ---------------------------------------------------------------------------

#include "cpc.utils.as"

// ---------------------------------------------------------------------------
//
// Export entry point.
//
// void Export(const string& in outputPath, SpriteExportContext@ ctx)
//
// ---------------------------------------------------------------------------

void Export(const string& in outputPath, SpriteExportContext@ ctx)
{
    int spriteCount = ctx.GetSpriteCount();
    string mode     = ctx.GetTargetMode();
    string system   = ctx.GetTargetSystem();
    Palette@ palette = ctx.GetPalette();
    bool writeHeader = ctx.GetParam("header") == "true";
    //
    // Validate that we have sprites to export
    //
    if (spriteCount <= 0) {
        Log_Error("Export failed: no sprites extracted -- run extraction first.");
        return;
    }
    //
    // Determine pixels per byte for the active mode
    //
    int ppb = 2;
    if (mode == "Mode 1") ppb = 4;
    else if (mode == "Mode 2") ppb = 8;
    Log_Info("Export started -- system: " + system + "  mode: " + mode
        + "  sprites: " + spriteCount
        + "  header: " + writeHeader
        + "  output: " + outputPath);
    //
    // Validate all sprites before writing any output
    //
    for (int i = 0; i < spriteCount; i++) {
        int w = ctx.GetSpriteWidth(i);
        if ((w % ppb) != 0) {
            Log_Error("Export failed: sprite #" + i + " (" + ctx.GetSpriteName(i)
                + ") width " + w + " is not a multiple of " + ppb
                + " for " + mode + ".");
            return;
        }
    }
    //
    // Encode all sprites into a single byte buffer
    //
    array<uint8> buf;
    //
    // Optional one-byte header: sprite count clamped to 255
    //
    if (writeHeader)
        buf.insertLast(uint8(spriteCount < 255 ? spriteCount : 255));
    //
    // Encode each sprite
    //
    bool useTransp = ctx.GetUseTransparentColor();
    int transpPen = ctx.GetTransparentPen();
    bool warnedNoTranspPen = false;
    bool warnedNoPen = false;
    for (int i = 0; i < spriteCount; i++) {
        Image@ img = ctx.GetSprite(i);
        if (img is null) {
            Log_Error("Export failed: GetSprite(" + i + ") returned null.");
            return;
        }
        int w = ctx.GetSpriteWidth(i);
        int h = ctx.GetSpriteHeight(i);
        int bytesPerRow = w / ppb;
        //
        // Two-byte size record so the loader can handle variable-size sprites
        //
        buf.insertLast(uint8(w));
        buf.insertLast(uint8(h));
        //
        // Encode pixel rows
        //
        array<int> penRow(w);
        for (int row = 0; row < h; row++) {
            //
            // Build a row of pen indices, applying transparent pixel detection
            //
            for (int col = 0; col < w; col++) {
                RgbColor px = img.GetPixelColor(col, row);
                //
                // Transparent pixel: alpha==0 (IsTransparent) or chroma-key enabled
                //
                bool isTransp = useTransp && px.IsTransparent();
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
                    pen = palette.PenGetIndex(px);
                    if (pen < 0) {
                        if (!warnedNoPen) {
                            Log_Warning("One or more pixels have no matching pen -- using pen 0.");
                            warnedNoPen = true;
                        }
                        pen = 0;
                    }
                }
                penRow[col] = pen;
            }
            //
            // Encode the row into bytes and append to the buffer
            //
            EncodePixels(penRow, 0, w, mode, buf);
        }
        Log_Info("Sprite #" + i + " '" + ctx.GetSpriteName(i)
            + "' -- " + w + "x" + h + " -> " + (h * bytesPerRow) + " bytes");
    }
    //
    // Write the buffer to disk
    //
    file@ f = file();
    if (f.open(outputPath, "w") < 0) {
        Log_Error("Export failed: cannot open output file: " + outputPath);
        return;
    }
    for (uint k = 0; k < buf.length(); k++)
        f.writeUInt(buf[k], 1);
    f.close();
    Log_Info("Export complete -- " + buf.length() + " bytes written to: " + outputPath);
}
