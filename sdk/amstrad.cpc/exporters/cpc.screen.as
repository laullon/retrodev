// ---------------------------------------------------------------------------
//
// Retrodev SDK
//
// CPC screen export script -- exports converted bitmap as CPC screen data.
//
// (c) TLOTB 2026
//
// ---------------------------------------------------------------------------
//
// Script metadata -- read by the tool at script selection time.
// All tags are optional but recommended. Lines are scanned as plain text;
// no compilation occurs when the tool reads these values.
// All the tags must be preceded by "@"
// 
// description <text>
//   One-line human-readable summary shown in the script picker UI.
//   Keep it concise: what this script produces and how.
//
// exporter <type>
//   Declares the kind of data this script exports.
//   The picker uses this to show only relevant scripts for the active document.
//   Known values:
//     bitmap   -- pixel/screen data (tile sheets, full screens, sprites)
//     map      -- tilemap or room layout data
//     sprites  -- individual sprite definitions
//     tiles    -- tile set / character set data
//
// target <system>
//   Declares the hardware this script targets.
//   The picker uses this to hide scripts that do not match the active
//   project target system (e.g. a ZX Spectrum script is hidden when the
//   project targets an Amstrad CPC).
//   The value is the system identifier, not the display name shown in the UI.
//   Known values:
//     amstrad.cpc  -- Amstrad CPC/CPC+
//     spectrum     -- ZX Spectrum
//     c64          -- Commodore 64
//     msx          -- MSX
//   Omit the tag entirely if the script is hardware-agnostic.
//
// param <key> <type> <default> <label...>
//   Declares a user-configurable parameter shown in the Export panel UI.
//   The tool builds one control per @param and stores the chosen values in
//   the project file.  At export time the script reads them via GetParam().
//   Supported types:
//     bool    -- checkbox;  default must be  true  or  false
//     int     -- integer spinner;  default is any decimal integer
//     string  -- text field;  default is a single word (no spaces)
//     combo   -- drop-down list;  format:  <key> combo <default> <opt1|opt2|...> <label...>
//               default must be one of the options
//   The label is everything after the default value; spaces are allowed.
//
// ---------------------------------------------------------------------------
//

// @description Exports CPC screen data, chunky or native screen memory layout.
// @exporter bitmap
// @target amstrad.cpc
// @param layout         combo  chunky  chunky|screenmemory             Memory layout
// @param exportPalette  bool   false                                   Export palette (.pal file)
// @param paletteFormat  combo  systemindex  systemindex|hardwareindex|hardwarecmd  Palette format

// ---------------------------------------------------------------------------
//
// Shared CPC utilities -- pixel encoding for all three video modes.
//
// cpc.utils.as provides:
//   EncodeByte(p0..p7, mode)  -- encodes one byte from up to 8 pen indices;
//                               use this directly when the tile or sprite
//                               ordering is not a plain horizontal run.
//   EncodePixels(pen, xStart, count, mode, buf)
//                             -- encodes a full horizontal run of pixels.
//
// Any CPC script can reuse these by adding the same #include line.
//
// ---------------------------------------------------------------------------

#include "cpc.utils.as"

// ---------------------------------------------------------------------------
//
// Main export entry point.
//
// Validates image dimensions for the active mode, then writes the pixel data
// either in chunky order (left-to-right, top-to-bottom) or in CPC screen
// memory layout (scanlines interleaved in groups of 8).
//
// CPC screen memory layout:
//   The CPC 16 KB screen is organised as 8 banks of 2048 bytes each.
//   Bank n contains scanlines n, n+8, n+16, ..., n+(h/8-1)*8 in order.
//   Any bytes remaining in the 2048-byte bank after the scanline data are
//   written as zero-padding, so the output file is always 8x2048 = 16384 bytes.
//   Example: Mode 0, 160x200 -> 80 bytes/line x 25 lines = 2000 bytes data
//            + 48 zero bytes padding per bank -> 16384 bytes total.
//
// ---------------------------------------------------------------------------
void Export(Image@ image, const string& in outputPath, BitmapExportContext@ ctx)
{
	int w    = ctx.GetNativeWidth();
	int h    = ctx.GetNativeHeight();
	string mode   = ctx.GetTargetMode();
	string system = ctx.GetTargetSystem();
	Palette@ palette = ctx.GetPalette();
	bool useScreenLayout = ctx.GetParam("layout") == "screenmemory";
	bool exportPalette   = ctx.GetParam("exportPalette") == "true";
	string paletteFormat = ctx.GetParam("paletteFormat");
	//
	// Validate width: must be a multiple of pixels-per-byte for the active mode
	//
	int ppb = 2;
	if (mode == "Mode 1") ppb = 4;
	else if (mode == "Mode 2") ppb = 8;
	if ((w % ppb) != 0) {
		Log_Error("Export failed: width " + w + " is not a multiple of " + ppb
			+ " for " + mode + ".");
		return;
	}
	//
	// Validate height: screen layout requires a multiple of 8
	//
	if (useScreenLayout && (h % 8) != 0) {
		Log_Error("Export failed: height " + h
			+ " must be a multiple of 8 for CPC screen memory layout.");
		return;
	}
	Log_Info("Export started -- system: " + system + "  mode: " + mode
		+ "  size: " + w + "x" + h
		+ "  screenlayout: " + useScreenLayout
		+ "  output: " + outputPath);
	//
	// Resolve every pixel's palette pen index up front so EncodePixels can
	// work with a plain integer array rather than calling back into the image.
	//
	bool useTransp = ctx.GetUseTransparentColor();
	int transpPen = ctx.GetTransparentPen();
	array<int> penMap(w * h);
	int penCount = palette.PaletteMaxColors();
	bool warnedNoTranspPen = false;
	bool warnedNoPen = false;
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			RgbColor c = image.GetPixelColor(x, y);
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
						Log_Warning("One or more pixels have no matching pen -- image may not have been converted. Using pen 0.");
						warnedNoPen = true;
					}
					pen = 0;
				}
			}
			penMap[y * w + x] = pen;
		}
	}
	//
	// Encode all scanlines and collect into a byte buffer
	//
	int bytesPerLine = w / ppb;
	array<uint8> buf;
	if (useScreenLayout) {
		//
		// CPC screen memory layout: 8 banks of 2048 bytes each.
		// Bank n holds scanlines n, n+8, ..., n+(linesPerBank-1)*8,
		// followed by zero-padding to fill the remaining bank bytes.
		//
		int linesPerBank = h / 8;
		int dataPerBank  = linesPerBank * bytesPerLine;
		int padPerBank   = 2048 - dataPerBank;
		if (padPerBank < 0) {
			Log_Error("Export failed: " + dataPerBank + " bytes per bank exceeds"
				+ " the 2048-byte CPC bank limit (image too wide for screen memory layout).");
			return;
		}
		buf.reserve(8 * 2048);
		for (int bank = 0; bank < 8; bank++) {
			//
			// Encode the scanlines that belong to this bank
			//
			for (int group = 0; group < linesPerBank; group++)
				EncodePixels(penMap, (bank + group * 8) * w, w, mode, buf);
			//
			// Zero-pad the remainder of the 2048-byte bank
			//
			for (int p = 0; p < padPerBank; p++)
				buf.insertLast(0);
		}
	} else {
		//
		// Chunky: sequential top-to-bottom, no padding
		//
		buf.reserve(h * bytesPerLine);
		for (int y = 0; y < h; y++)
			EncodePixels(penMap, y * w, w, mode, buf);
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
	Log_Info("Export finished -- " + buf.length() + " bytes written.");
	//
	// Palette export -- one byte per pen, written to <outputPath base>.pal
	//
	if (exportPalette) {
		//
		// Build the palette byte buffer
		//
		array<uint8> palBuf;
		for (int pen = 0; pen < penCount; pen++) {
			int sysIdx = palette.PenGetColorIndex(pen);
			uint8 palByte;
			if (paletteFormat == "hardwareindex") {
				int hwIdx = CpcHardwareColorIndex(sysIdx);
				palByte = uint8(hwIdx >= 0 ? hwIdx : 0);
			} else if (paletteFormat == "hardwarecmd") {
				int cmdIdx = CpcHardwareCmdColorIndex(sysIdx);
				palByte = uint8(cmdIdx >= 0 ? cmdIdx : 0);
			} else {
				palByte = uint8(sysIdx >= 0 ? sysIdx : 0);
			}
			palBuf.insertLast(palByte);
		}
		//
		// Derive the .pal output path by replacing the extension of outputPath
		//
		string palPath = outputPath;
		int dotPos = palPath.findLast(".");
		if (dotPos >= 0)
			palPath = palPath.substr(0, dotPos) + ".pal";
		else
			palPath = palPath + ".pal";
		//
		// Write the palette bytes to disk
		//
		file@ pf = file();
		if (pf.open(palPath, "w") < 0) {
			Log_Error("Palette export failed: could not open output file: " + palPath);
		} else {
			for (uint i = 0; i < palBuf.length(); i++)
				pf.writeUInt(palBuf[i], 1);
			pf.close();
			Log_Info("Palette exported -- " + palBuf.length() + " bytes written to: " + palPath);
		}
	}
}
