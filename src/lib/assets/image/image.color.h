// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Image asset -- per-pixel colour operations and conversions.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <cstdint>

namespace RetrodevLib {

	struct RgbColor {
		uint8_t r, g, b, a;
		//
		// Default constructor: black, fully opaque
		//
		RgbColor() : r(0), g(0), b(0), a(255) {}
		//
		// RGB constructor: alpha defaults to fully opaque for backward compatibility
		//
		RgbColor(uint8_t compR, uint8_t compG, uint8_t compB);
		//
		// RGBA constructor: explicit alpha control
		//
		RgbColor(uint8_t compR, uint8_t compG, uint8_t compB, uint8_t compA);
		//
		// Integer constructor: extracts RGB from packed int, alpha defaults to opaque
		//
		RgbColor(int value);
		//
		// Returns packed RGB color (24-bit)
		//
		int GetColor() const;
		//
		// Returns packed ARGB color (32-bit with alpha)
		//
		int GetColorArgb() const;
		//
		// Returns true if pixel is fully transparent (alpha = 0)
		//
		bool IsTransparent() const;
		//
		// Returns true if pixel is fully opaque (alpha = 255)
		//
		bool IsOpaque() const;
	};
}