// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Palette conversion interface.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once
#include <vector>
#include <string>
#include <assets/image/image.color.h>

namespace RetrodevLib {

	//
	// Interface to store and fetch palette values for color conversion
	//
	class IPaletteConverter {
	public:
		virtual ~IPaletteConverter() = default;

		//
		// Returns the maximum number of colors in the system palette (e.g., 27 for CPC, 4096 for CPC+)
		//
		virtual int GetSystemMaxColors() = 0;

		//
		// Returns the RGB value for the given index in the system palette
		// index: system palette index
		// Returns: RGB color
		//
		virtual RgbColor GetSystemColorByIndex(int index) = 0;

		//
		// Returns an index to the complete palette using the given RGB values
		// color: RGB values
		// colorSelectionMode: Color selection mode string
		// Returns: index to the palette
		//
		virtual int GetSystemIndexByColor(const RgbColor& color, const std::string& colorSelectionMode) = 0;

		//
		// Returns the maximum number of colors that could be present in the palette
		// in the current configuration. In case of the CPC in mode 0 16 colors
		//
		virtual int PaletteMaxColors() = 0;

		//
		// Returns the maximum number of colors that could be present in the palette for the given line
		// In case of the CPC and CPC+, depends on the target video mode
		// line: line index (y coordinate) This is used for video modes that alternate color schemas per line,
		// like the CPC EGX ones.
		//
		virtual int PaletteMaxColorsByLine(int line) = 0;

		//
		// Returns the RGB color pointed by the given palette index (pen)
		// index: palette index
		// Returns: RGB color
		//
		virtual RgbColor PenGetColor(int index) = 0;

		//
		// Returns the pen index whose assigned color matches the given RGB color,
		// or -1 if no pen in the active palette holds that color.
		// Only searches within the active pen slots (PaletteMaxColors), not the full system palette.
		// color: RGB color to search for
		// Returns: pen index (0-based), or -1 if not found
		//
		virtual int PenGetIndex(const RgbColor& color) = 0;

		//
		// Set the system color index into the given palette index (pen)
		// pen: palette index
		// index: system color index
		//
		virtual void PenSetColorIndex(int pen, int index) = 0;

		//
		// Return the system color index by the given palette index
		// pen: palette index
		// Returns: system color index
		//
		virtual int PenGetColorIndex(int pen) = 0;

		//
		// Lock/unlock a palette index (pen)
		// Locked pens can be used but their color cannot be changed during quantization
		// pen: palette index
		// lock: true to lock, false to unlock
		//
		virtual void PenLock(int pen, bool lock) = 0;
		//
		// Enable/disable a palette index (pen)
		// Disabled pens cannot be used at all during quantization
		// pen: palette index
		// enable: true to enable, false to disable
		//
		virtual void PenEnable(int pen, bool enable) = 0;
		//
		// Returns whether a palette index (pen) is locked
		// pen: palette index
		// Returns: true if locked, false otherwise
		//
		virtual bool PenGetLock(int pen) = 0;
		//
		// Returns whether a palette index (pen) is enabled
		// pen: palette index
		// Returns: true if enabled, false if disabled
		//
		virtual bool PenGetEnabled(int pen) = 0;
		//
		// Returns whether a palette index (pen) has been assigned a color since the last reset.
		// A pen that has never been touched by the quantizer or by PenSetColorIndex returns false.
		// This is distinct from PenGetColor which returns black for both unset and black-assigned pens.
		// pen: palette index
		// Returns: true if the pen has been explicitly assigned, false if it is still in its initial state
		//
		virtual bool PenGetUsed(int pen) = 0;

		//
		// Returns the list of supported color selection modes
		//
		virtual std::vector<std::string> GetColorSelectionModes() = 0;

		//
		// Returns the color distance between two colors
		// c1: first color
		// c2: second color
		// Returns: distance value
		//
		virtual int ColorDistance(const RgbColor& c1, const RgbColor& c2) = 0;
		//
		// Lock or unlock all pens at once
		// lock: true to lock all, false to unlock all
		//
		virtual void PenLockAll(bool lock) = 0;
		//
		// Enable or disable all pens at once
		// enable: true to enable all, false to disable all
		//
		virtual void PenEnableAll(bool enable) = 0;
	};
}
