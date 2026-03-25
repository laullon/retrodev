// ---------------------------------------------------------------------------
//
// Retrodev SDK
//
// CPC export utilities -- shared pixel encoding helpers for all CPC modes.
//
// (c) TLOTB 2026
//
// ---------------------------------------------------------------------------
//
// Script metadata
//
// @description Utils to export CPC graphic data.
// @exporter util
// @target amstrad.cpc
//
// ---------------------------------------------------------------------------
//
// cpc.utils.as -- Amstrad CPC pixel encoding utilities
//
// Include this file from any CPC export script to get hardware-accurate
// pixel-to-byte encoding for all three video modes:
//
//   #include "cpc.utils.as"
//
// Provided functions
// ------------------
//
//   uint8 EncodeByte(int p0, int p1, int p2, int p3,
//                    int p4, int p5, int p6, int p7,
//                    const string& in mode)
//
//     Encodes up to 8 pen indices into a single CPC byte for the given mode.
//     Unused pixel slots (beyond the mode's pixel-per-byte count) are ignored.
//     This is the primitive unit -- use it when encoding sprites, tiles, or any
//     layout that works one byte at a time (e.g. zigzag ordering).
//
//   void EncodePixels(array<int>@ pen, int xStart, int count,
//                     const string& in mode, array<uint8>@ buf)
//
//     Encodes a horizontal run of 'count' pen indices starting at pen[xStart]
//     and appends the resulting bytes to buf.  Built on top of EncodeByte.
//     Use this for full scanlines.
//
//   int CpcHardwareColorIndex(int sysIndex)
//
//     Converts a firmware color index (0-26) to the raw hardware color index
//     (0-31) used by the Gate Array.
//
//   int CpcHardwareCmdColorIndex(int sysIndex)
//
//     Converts a firmware color index (0-26) to the Gate Array ink register
//     command byte (hardware index | 0x40), ready to write to port 0x7F.
//
// CPC hardware pixel encoding per mode
// -------------------------------------
//
//   Mode 0  (16 colours, 2 pixels per byte)
//     Pixel 0 pen bits -> display byte bits:  bit3->1, bit2->5, bit1->3, bit0->7
//     Pixel 1 pen bits -> display byte bits:  bit3->0, bit2->4, bit1->2, bit0->6
//     byte: [p0b0][p1b0][p0b2][p1b2][p0b1][p1b1][p0b3][p1b3]
//
//   Mode 1  (4 colours, 4 pixels per byte)
//     Pixel 0 pen bits -> display byte bits:  bit1->3, bit0->7
//     Pixel 1 pen bits -> display byte bits:  bit1->2, bit0->6
//     Pixel 2 pen bits -> display byte bits:  bit1->1, bit0->5
//     Pixel 3 pen bits -> display byte bits:  bit1->0, bit0->4
//     byte: [p0b0][p1b0][p2b0][p3b0][p0b1][p1b1][p2b1][p3b1]
//
//   Mode 2  (2 colours, 8 pixels per byte)
//     P0->bit7, P1->bit6, P2->bit5, P3->bit4, P4->bit3, P5->bit2, P6->bit1, P7->bit0
//     byte: [p0][p1][p2][p3][p4][p5][p6][p7]
//
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
//
// EncodeByte -- encode one CPC byte from up to 8 pen indices.
//
// Parameters p0..p7 are pen indices; pass 0 for any unused slots.
// Only the number of pixels relevant to the mode are read:
//   Mode 0 uses p0, p1
//   Mode 1 uses p0..p3
//   Mode 2 uses p0..p7
//
// ---------------------------------------------------------------------------
uint8 EncodeByte(int p0, int p1, int p2, int p3,
                 int p4, int p5, int p6, int p7,
                 const string& in mode)
{
    if (mode == "Mode 0") {
        //
        // Pixel 0 pen bits -> display byte bits:  bit3->1, bit2->5, bit1->3, bit0->7
        // Pixel 1 pen bits -> display byte bits:  bit3->0, bit2->4, bit1->2, bit0->6
        //
        return uint8(
            ((p0 & 1) << 7) | ((p1 & 1) << 6) |
            ((p0 & 4) << 3) | ((p1 & 4) << 2) |
            ((p0 & 2) << 2) | ((p1 & 2) << 1) |
            ((p0 & 8) >> 2) | ((p1 & 8) >> 3)
        );
    }
    if (mode == "Mode 1") {
        //
        // Pixel 0 pen bits -> display byte bits:  bit1->3, bit0->7
        // Pixel 1 pen bits -> display byte bits:  bit1->2, bit0->6
        // Pixel 2 pen bits -> display byte bits:  bit1->1, bit0->5
        // Pixel 3 pen bits -> display byte bits:  bit1->0, bit0->4
        //
        return uint8(
            ((p0 & 1) << 7) | ((p1 & 1) << 6) |
            ((p2 & 1) << 5) | ((p3 & 1) << 4) |
            ((p0 & 2) << 2) | ((p1 & 2) << 1) |
             (p2 & 2)       | ((p3 & 2) >> 1)
        );
    }
    //
    // Mode 2: one bit per pixel, pixels 0-7 in bits 7-0
    //
    uint8 b = 0;
    if ((p0 & 1) != 0) b = b | 0x80;
    if ((p1 & 1) != 0) b = b | 0x40;
    if ((p2 & 1) != 0) b = b | 0x20;
    if ((p3 & 1) != 0) b = b | 0x10;
    if ((p4 & 1) != 0) b = b | 0x08;
    if ((p5 & 1) != 0) b = b | 0x04;
    if ((p6 & 1) != 0) b = b | 0x02;
    if ((p7 & 1) != 0) b = b | 0x01;
    return b;
}

// ---------------------------------------------------------------------------
//
// EncodePixels -- encode a horizontal run of pixels and append to buf.
//
// pen    : flat pen-index array (typically the full-image penMap)
// xStart : index of the first pixel in pen[] to encode
// count  : number of pixels to encode (must be a multiple of ppb for the mode)
// mode   : target video mode string
// buf    : destination byte array; bytes are appended
//
// ---------------------------------------------------------------------------
void EncodePixels(array<int>@ pen, int xStart, int count,
                  const string& in mode, array<uint8>@ buf)
{
    if (mode == "Mode 0") {
        for (int i = 0; i < count; i += 2) {
            int p0 = pen[xStart + i];
            int p1 = (i + 1 < count) ? pen[xStart + i + 1] : 0;
            buf.insertLast(EncodeByte(p0, p1, 0, 0, 0, 0, 0, 0, mode));
        }
    } else if (mode == "Mode 1") {
        for (int i = 0; i < count; i += 4) {
            int p0 = pen[xStart + i];
            int p1 = (i + 1 < count) ? pen[xStart + i + 1] : 0;
            int p2 = (i + 2 < count) ? pen[xStart + i + 2] : 0;
            int p3 = (i + 3 < count) ? pen[xStart + i + 3] : 0;
            buf.insertLast(EncodeByte(p0, p1, p2, p3, 0, 0, 0, 0, mode));
        }
    } else {
        //
        // Mode 2: eight pixels per byte
        //
        for (int i = 0; i < count; i += 8) {
            int p0 = pen[xStart + i];
            int p1 = (i + 1 < count) ? pen[xStart + i + 1] : 0;
            int p2 = (i + 2 < count) ? pen[xStart + i + 2] : 0;
            int p3 = (i + 3 < count) ? pen[xStart + i + 3] : 0;
            int p4 = (i + 4 < count) ? pen[xStart + i + 4] : 0;
            int p5 = (i + 5 < count) ? pen[xStart + i + 5] : 0;
            int p6 = (i + 6 < count) ? pen[xStart + i + 6] : 0;
            int p7 = (i + 7 < count) ? pen[xStart + i + 7] : 0;
            buf.insertLast(EncodeByte(p0, p1, p2, p3, p4, p5, p6, p7, mode));
        }
    }
}

// ---------------------------------------------------------------------------
//
// CpcHardwareColorIndex -- convert a CPC firmware color index (0-26) to the
// raw hardware color index (0-31) used by the Gate Array.
//
// Returns -1 for an out-of-range firmware index.
//
// Firmware | Hardware | Colour Name
// ---------------------------------------------------------------------------
//      0   |  20      | Black
//      1   |   4      | Blue
//      2   |  21      | Bright Blue
//      3   |  28      | Red
//      4   |  24      | Magenta
//      5   |  29      | Mauve
//      6   |  12      | Bright Red
//      7   |   5      | Purple
//      8   |  13      | Bright Magenta
//      9   |  22      | Green
//     10   |   6      | Cyan
//     11   |  23      | Sky Blue
//     12   |  30      | Yellow
//     13   |   0      | White
//     14   |  31      | Pastel Blue
//     15   |  14      | Orange
//     16   |   7      | Pink
//     17   |  15      | Pastel Magenta
//     18   |  18      | Bright Green
//     19   |   2      | Sea Green
//     20   |  19      | Bright Cyan
//     21   |  26      | Lime
//     22   |  25      | Pastel Green
//     23   |  27      | Pastel Cyan
//     24   |  10      | Bright Yellow
//     25   |   3      | Pastel Yellow
//     26   |  11      | Bright White
// ---------------------------------------------------------------------------
int CpcHardwareColorIndex(int sysIndex)
{
    if (sysIndex < 0 || sysIndex > 26)
        return -1;
    array<int> hwTable = {
        20,  4, 21, 28, 24, 29, 12,  5, 13,
        22,  6, 23, 30,  0, 31, 14,  7, 15,
        18,  2, 19, 26, 25, 27, 10,  3, 11
    };
    return hwTable[sysIndex];
}

// ---------------------------------------------------------------------------
//
// CpcHardwareCmdColorIndex -- convert a CPC firmware color index (0-26) to the
// Gate Array ink register command byte (hardware index | 0x40).
//
// Some entries have a known alternate command byte (noted in comments);
// the primary value is used here.
//
// Returns -1 for an out-of-range firmware index.
//
// Firmware | Cmd byte | Alt  | Colour Name
// ---------------------------------------------------------------------------
//      0   |   0x54   |      | Black
//      1   |   0x44   | 0x50 | Blue
//      2   |   0x55   |      | Bright Blue
//      3   |   0x5C   |      | Red
//      4   |   0x58   |      | Magenta
//      5   |   0x5D   |      | Mauve
//      6   |   0x4C   |      | Bright Red
//      7   |   0x45   | 0x48 | Purple
//      8   |   0x4D   |      | Bright Magenta
//      9   |   0x56   |      | Green
//     10   |   0x46   |      | Cyan
//     11   |   0x57   |      | Sky Blue
//     12   |   0x5E   |      | Yellow
//     13   |   0x40   | 0x41 | White
//     14   |   0x5F   |      | Pastel Blue
//     15   |   0x4E   |      | Orange
//     16   |   0x47   |      | Pink
//     17   |   0x4F   |      | Pastel Magenta
//     18   |   0x52   |      | Bright Green
//     19   |   0x42   | 0x51 | Sea Green
//     20   |   0x53   |      | Bright Cyan
//     21   |   0x5A   |      | Lime
//     22   |   0x59   |      | Pastel Green
//     23   |   0x5B   |      | Pastel Cyan
//     24   |   0x4A   |      | Bright Yellow
//     25   |   0x43   | 0x49 | Pastel Yellow
//     26   |   0x4B   |      | Bright White
// ---------------------------------------------------------------------------
int CpcHardwareCmdColorIndex(int sysIndex)
{
    if (sysIndex < 0 || sysIndex > 26)
        return -1;
    array<int> hwTable = {
        0x54, 0x44, 0x55, 0x5C, 0x58, 0x5D, 0x4C, 0x45, 0x4D,
        0x56, 0x46, 0x57, 0x5E, 0x40, 0x5F, 0x4E, 0x47, 0x4F,
        0x52, 0x42, 0x53, 0x5A, 0x59, 0x5B, 0x4A, 0x43, 0x4B
    };
    return hwTable[sysIndex];
}
