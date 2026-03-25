// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Palette asset parameters -- pen count and colour definitions.
//
// (c) TLOTB 2026
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
		// Always present within this zone across all levels (zone-scoped permanent participant).
		// Unlike Always, colors are not shared across zones -- they are added on top of the
		// global Always base for this zone only.
		//
		ZoneAlways
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
		// Level tag -- only used when role is Level.
		// Groups Level participants that appear together (e.g. "Level1", "World2").
		// Ignored for Always and ZoneAlways roles.
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
		// Target video mode for this zone (e.g. "Mode 0") -- allows different zones
		// to use different screen modes (e.g. Mode 0 gameplay area + Mode 1 status bar).
		//
		std::string targetMode;
		//
		// Graphics items that participate in this zone
		//
		std::vector<PaletteParticipant> participants;
	};
	//
	// Method used to handle the palette when the number of unique colors across all
	// participants exceeds the hardware pen budget.
	//
	enum class PaletteOverflowMethod {
		//
		// Truncate the union list at pensAvailable.
		// Priority order (Always > Zone > Level) ensures the most important colors survive.
		// Colors beyond the cap are dropped; participants using them are remapped to the
		// nearest surviving color during the final forced conversion pass.
		//
		HardCap,
		//
		// For each overflow color, find the accepted entry it is closest to (by RGB distance),
		// then replace that accepted entry with the system color nearest to the 50/50 RGB
		// midpoint of the two. Accepted colors are pulled toward their overflow neighbors,
		// packing more perceptual variety into the available pens at the cost of accuracy.
		//
		SoftCap,
		//
		// Like SoftCap but the blend is 67% accepted + 33% overflow. The accepted palette
		// shifts only slightly toward overflow colors, preserving dominant color fidelity
		// better while still gaining some perceptual coverage from overflow entries.
		//
		WeightedBlend,
		//
		// Cluster each overflow color with the accepted entry it is nearest to, then replace
		// that accepted entry with the RGB centroid (equal-weight average) of the full cluster.
		// When multiple overflow colors compete for the same accepted entry the centroid absorbs
		// all of them, spreading the compromise evenly across the whole group.
		//
		Median
	};
	//
	// Top-level parameters for a palette build item.
	// Stores the target hardware configuration and all screen zones.
	//
	struct PaletteParams {
		//
		// Target system identifier (e.g. "Amstrad CPC") -- mirrors ConvertParams::TargetSystem
		//
		std::string targetSystem;
		//
		// Target palette type (e.g. "Hardware", "Plus") -- mirrors ConvertParams::PaletteType
		//
		std::string targetPaletteType;
		//
		// Method used to handle the palette when it overflows the pen budget
		//
		PaletteOverflowMethod overflowMethod = PaletteOverflowMethod::HardCap;
		//
		// Screen zones (at least one is always present)
		//
		std::vector<PaletteZone> zones;
		//
		// Pre-loaded pen slots: system color indices the user has manually assigned and locked
		// before solving. Index i gives the system color index for pen slot i (-1 = unassigned).
		// Only locked, assigned slots are injected at the head of the global Always union so
		// they always occupy the first pen slots in every solved palette.
		//
		std::vector<int> preloadedColors;
		//
		// Per-slot lock flag for pre-loaded slots.
		// Only slots where preloadedLocked[i] == true and preloadedColors[i] >= 0 are injected
		// into the solver as immutable colors.
		//
		std::vector<bool> preloadedLocked;
		//
		// Set to true when the user explicitly clicks Validate after reviewing the solution.
		// When true the build pipeline applies the palette assignments even if the solution
		// is imperfect (e.g. colors were remapped due to pen overflow).
		// Cleared automatically when any participant or palette type changes.
		//
		bool userValidated = false;
	};
}
