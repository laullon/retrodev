// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Sprite conversion parameters.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <string>
#include <vector>

namespace RetrodevLib {
	//
	// Individual sprite definition
	// Defines the position and size of a single sprite within the source image
	//
	struct SpriteDefinition {
		int X = 0;
		int Y = 0;
		int Width = 16;
		int Height = 16;
		std::string Name = "";
		//
		// Flip transforms applied to the extracted pixel data.
		// FlipH mirrors pixels left-to-right; FlipV mirrors pixels top-to-bottom.
		// Both can be set simultaneously for a 180-degree rotation.
		//
		bool FlipH = false;
		bool FlipV = false;
		//
		// Sub-pixel shift applied after extraction.
		// Pixels are shifted by (ShiftX, ShiftY) with wrap-around so the sprite
		// content is cyclically scrolled within its bounding box.
		// Useful for generating pre-shifted variants for systems with coarse
		// pixel-per-byte encoding (e.g. CPC Mode 0: 2px/byte).
		//
		int ShiftX = 0;
		int ShiftY = 0;
	};
	//
	// Sprite extraction parameters
	// Contains a list of sprite definitions to extract from the source image
	//
	struct SpriteExtractionParams {
		//
		// List of sprite definitions (position, size, name)
		//
		std::vector<SpriteDefinition> Sprites;
	};
}
