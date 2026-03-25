// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Utility functions -- string, file path and general helpers.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <cstdint>

namespace RetrodevLib {

	//
	// Utility functions for common operations used across the library
	//
	class Utils {
	public:
		//
		// Clamp an integer value to [0, max]
		//
		static inline int Clamp(int v, int max) { return v < 0 ? 0 : (v > max ? max : v); }
		//
		// Clamp a float value to [lo, hi]
		//
		static inline float Clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
		//
		// Clamp a float value to byte range [0, 255]
		//
		static inline uint8_t MinMaxByte(float val) { return static_cast<uint8_t>(val < 0.0f ? 0.0f : (val > 255.0f ? 255.0f : val)); }
		//
		// Clamp a double value to byte range [0, 255]
		//
		static inline uint8_t MinMaxByte(double val) { return static_cast<uint8_t>(val < 0.0 ? 0.0 : (val > 255.0 ? 255.0 : val)); }
		//
		// Sample a pixel from RGBA data with boundary clamping
		// Returns pointer to the pixel at (x, y) with clamping to image bounds
		// stride: bytes per row (usually width * 4)
		//
		static inline const uint8_t* SamplePixel(const uint8_t* src, int x, int y, int w, int h, int stride) {
			x = Clamp(x, w - 1);
			y = Clamp(y, h - 1);
			return src + y * stride + x * 4;
		}
	};

}
