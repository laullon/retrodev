// ---------------------------------------------------------------------------
//
// Script metadata
//
// @description Exports map layers as raw binary data, one file per layer.
// @exporter map
// @param include_empty  bool  false  Include layers with no tile data
// @param layer_header   bool  true   Prepend per-layer width/height header
//
// ---------------------------------------------------------------------------
//
// Output format
// -------------
//
// Each layer is written to its own file.  When the map has more than one
// layer the output filename is suffixed with the layer index before the
// extension:  e.g.  map.bin  ->  map_0.bin,  map_1.bin, ...
// Single-layer maps write to the original output path unchanged.
//
// Layers with all-zero data are skipped unless include_empty is true.
//
// When layer_header is true, each file begins with a 4-byte header:
//   byte 0-1  — layer width  as a little-endian uint16
//   byte 2-3  — layer height as a little-endian uint16
//
// The tile data follows immediately.  Each cell is stored as a
// little-endian uint16 (the raw cell word from the map):
//   bits 15-12  tilesetSlotIndex + 1  (0 = empty cell, 1-15 = slot index)
//   bits 11-0   tileIndex within that slot's active tileset (0-4095)
//
// The total number of uint16 values per layer is width * height.
//
// ---------------------------------------------------------------------------

string MakeLayerPath(const string& in basePath, int layerIndex, int layerCount)
{
    if (layerCount <= 1)
        return basePath;
    //
    // Split at the last '.' to insert the layer index before the extension
    //
    int dot = int(basePath.length()) - 1;
    while (dot >= 0 && basePath[dot] != 46)
        dot--;
    if (dot < 0)
        return basePath + "_" + layerIndex;
    return basePath.substr(0, dot) + "_" + layerIndex + basePath.substr(dot);
}

void Export(const string& in outputPath, MapExportContext@ ctx)
{
    int layerCount      = ctx.GetLayerCount();
    bool includeEmpty   = ctx.GetParam("include_empty") == "true";
    bool writeHeader    = ctx.GetParam("layer_header")  == "true";

    if (layerCount <= 0) {
        Log_Error("Export failed: map has no layers.");
        return;
    }

    Log_Info("Export started — layers: " + layerCount
        + "  include_empty: " + includeEmpty
        + "  layer_header: " + writeHeader
        + "  output: " + outputPath);

    for (int li = 0; li < layerCount; li++) {
        int w = ctx.GetLayerWidth(li);
        int h = ctx.GetLayerHeight(li);
        string name = ctx.GetLayerName(li);
        //
        // Check whether this layer has any non-empty cells
        //
        if (!includeEmpty) {
            bool hasData = false;
            for (int row = 0; row < h && !hasData; row++) {
                for (int col = 0; col < w && !hasData; col++) {
                    if (ctx.GetCell(li, col, row) != 0)
                        hasData = true;
                }
            }
            if (!hasData) {
                Log_Info("Skipping empty layer " + li + " (" + name + ")");
                continue;
            }
        }
        //
        // Build the output path for this layer
        //
        string layerPath = MakeLayerPath(outputPath, li, layerCount);
        Log_Info("Writing layer " + li + " (" + name + ")  " + w + "x" + h + "  ->  " + layerPath);
        //
        // Assemble the layer bytes into a buffer
        //
        array<uint8> buf;
        if (writeHeader) {
            buf.insertLast(uint8(w & 0xFF));
            buf.insertLast(uint8((w >> 8) & 0xFF));
            buf.insertLast(uint8(h & 0xFF));
            buf.insertLast(uint8((h >> 8) & 0xFF));
        }
        for (int row = 0; row < h; row++) {
            for (int col = 0; col < w; col++) {
                int cell = ctx.GetCell(li, col, row);
                buf.insertLast(uint8(cell & 0xFF));
                buf.insertLast(uint8((cell >> 8) & 0xFF));
            }
        }
        //
        // Write the buffer to disk
        //
        file f;
        if (f.open(layerPath, "w") < 0) {
            Log_Error("Export failed: could not open output file: " + layerPath);
            return;
        }
        for (uint i = 0; i < buf.length(); i++)
            f.writeUInt(buf[i], 1);
        f.close();
        Log_Info("Layer " + li + " done — " + buf.length() + " bytes written.");
    }
    Log_Info("Export complete.");
}
