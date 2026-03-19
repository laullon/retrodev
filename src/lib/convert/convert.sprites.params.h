// --------------------------------------------------------------------------------------------------------------
//
//
//
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
} // namespace RetrodevLib
