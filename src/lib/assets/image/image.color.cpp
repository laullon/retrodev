//-----------------------------------------------------------------------------------------------------------------------
//
//
//
//
//
//
//
//-----------------------------------------------------------------------------------------------------------------------

#include <assets/image/image.color.h>

namespace RetrodevLib {
	//
	// RGB constructor: alpha defaults to fully opaque (255)
	//
	RgbColor::RgbColor(uint8_t compR, uint8_t compG, uint8_t compB) : r(compR), g(compG), b(compB), a(255) {}

	//
	// RGBA constructor: explicit alpha control
	//
	RgbColor::RgbColor(uint8_t compR, uint8_t compG, uint8_t compB, uint8_t compA) : r(compR), g(compG), b(compB), a(compA) {}

	//
	// Integer constructor: extracts RGB from packed value, alpha defaults to opaque
	//
	RgbColor::RgbColor(int value) {
		r = (value >> 16) & 0xFF;
		g = (value >> 8) & 0xFF;
		b = value & 0xFF;
		a = 255; // Default to opaque
	}

	//
	// Returns packed RGB color (24-bit, no alpha)
	//
	int RgbColor::GetColor() const {
		return b + (g << 8) + (r << 16);
	}

	//
	// Returns packed ARGB color (32-bit with alpha channel)
	//
	int RgbColor::GetColorArgb() const {
		return b + (g << 8) + (r << 16) + (a << 24);
	}

	//
	// Returns true if pixel is fully transparent (can be excluded from processing)
	//
	bool RgbColor::IsTransparent() const {
		return a == 0;
	}

	//
	// Returns true if pixel is fully opaque
	//
	bool RgbColor::IsOpaque() const {
		return a == 255;
	}
} // namespace RetrodevLib