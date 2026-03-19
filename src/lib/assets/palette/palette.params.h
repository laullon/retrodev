// --------------------------------------------------------------------------------------------------------------
//
//
//
//
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <convert/convert.params.h>
#include <string>
#include <vector>

namespace RetrodevLib {
	//
	// Describes the role a graphics participant plays in the palette zone.
	// This drives which conversions must fit inside the zone's pen budget.
	//
	enum class PaletteParticipantRole {
		//
		// Present on every frame for the lifetime of the program (e.g. HUD, status bar)
		//
		Always,
		//
		// Present only during a specific game level (swapped in when that level loads)
		//
		Level,
		//
		// Present only while the raster beam is inside the zone's scanline range
		//
		ScreenZone
	};
	//
	// One graphics item that participates in a palette zone.
	// It references a project build item by name (bitmap, tileset, or sprite entry).
	//
	struct PaletteParticipant {
		//
		// Project build item name (e.g. "amstrad.cpc/player", "tiles/forest")
		//
		std::string buildItemName;
		//
		// Build item type as a string ("Bitmap", "Tilemap", "Sprite") for serialisation
		// and display purposes. The type determines which project collection to search.
		//
		std::string buildItemType;
		//
		// Role this participant plays in the zone
		//
		PaletteParticipantRole role = PaletteParticipantRole::Always;
		//
		// Optional free-text tag (e.g. a level name, a zone nickname).
		// Ignored when role is Always.
		//
		std::string tag;
	};
	//
	// A screen zone is a horizontal scanline band that shares a single palette.
	// For a standard full-screen game there is one zone (lines 0-199 on CPC).
	// Split-screen effects (status bars, raster tricks) use multiple zones.
	//
	struct PaletteZone {
		//
		// Display name for the zone (e.g. "Game Area", "Status Bar")
		//
		std::string name;
		//
		// First scanline of the zone (inclusive, 0-based)
		//
		int lineStart = 0;
		//
		// Last scanline of the zone (inclusive)
		//
		int lineEnd = 199;
		//
		// Target video mode for this zone (e.g. "Mode 0") — allows different zones
		// to use different screen modes (e.g. Mode 0 gameplay area + Mode 1 status bar).
		//
		std::string targetMode;
		//
		// Graphics items that participate in this zone
		//
		std::vector<PaletteParticipant> participants;
	};
	//
	// Method used to cap the union palette when the number of unique colors across all
	// participants exceeds the hardware pen budget.
	//
	enum class PaletteCapMethod {
		//
		// Truncate the union list at pensAvailable.
		// Priority order (Always > Zone > Level) ensures the most important colors survive.
		// Colors beyond the cap are dropped; participants using them are remapped to the
		// nearest surviving color during the final forced conversion pass.
		//
		HardCap
	};
	//
	// Top-level parameters for a palette build item.
	// Stores the target hardware configuration and all screen zones.
	//
	struct PaletteParams {
		//
		// Target system identifier (e.g. "Amstrad CPC") — mirrors ConvertParams::TargetSystem
		//
		std::string targetSystem;
		//
		// Target palette type (e.g. "Hardware", "Plus") — mirrors ConvertParams::PaletteType
		//
		std::string targetPaletteType;
		//
		// Method used to reduce the union palette when it overflows the pen budget
		//
		PaletteCapMethod capMethod = PaletteCapMethod::HardCap;
		//
		// Screen zones (at least one is always present)
		//
		std::vector<PaletteZone> zones;
	};
} // namespace RetrodevLib
