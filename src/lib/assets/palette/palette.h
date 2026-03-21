// --------------------------------------------------------------------------------------------------------------
//
//
//
//
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <assets/palette/palette.params.h>
#include <convert/convert.palette.h>
#include <convert/convert.bitmap.h>
#include <convert/convert.bitmap.params.h>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace RetrodevLib {
	//
	// Result status for a single participant after attempting to fit it into a zone palette
	//
	enum class PaletteParticipantStatus {
		//
		// Participant was successfully quantized and all its colors fit in the zone palette
		//
		OK,
		//
		// The build item referenced by this participant was not found in the project
		//
		Missing,
		//
		// The source image for this build item could not be loaded from disk
		//
		ImageLoadFailed,
		//
		// The participant's colors could not all fit — the zone ran out of free pen slots
		//
		PenOverflow,
		//
		// The participant was skipped because it does not apply to the current solve context
		// (e.g. a Level participant whose tag does not match the active level filter)
		//
		Skipped
	};

	//
	// Per-participant result produced by the solver for one zone solve pass
	//
	struct PaletteParticipantResult {
		//
		// Index into PaletteZone::participants this result corresponds to
		//
		int participantIndex = -1;
		//
		// Solve outcome for this participant
		//
		PaletteParticipantStatus status = PaletteParticipantStatus::Missing;
		//
		// Number of new pen slots consumed by this participant (0 if Skipped or failed)
		//
		int pensConsumed = 0;
		//
		// Number of colors in the source image that could not be mapped to any free pen
		// (0 unless status is PenOverflow)
		//
		int unmappedColors = 0;
		//
		// Human-readable message summarising the result for display in the UI
		//
		std::string message;
		//
		// Overflow method that was applied when producing the fixed palette this participant
		// was mapped against. Transient — not serialised.
		//
		PaletteOverflowMethod overflowMethodApplied = PaletteOverflowMethod::HardCap;
		//
		// Colors actually used by this participant in the final fixed palette.
		// Populated only when status == OK. Transient — not serialised.
		//
		struct ColorSwatch {
			int slot = 0;
			int colorIndex = -1;
			uint8_t r = 0, g = 0, b = 0;
		};
		std::vector<ColorSwatch> assignedColors;
	};

	//
	// Solved palette for one (zone x level-tag) combination.
	// The tag is empty ("") for the base (Always-only) palette and for Zone/ScreenZone passes.
	// Level solutions are built on top of the zone base so their pen slots are additive.
	//
	struct PaletteTagSolution {
		//
		// The tag this solution was solved for.
		// "" means the Always + Zone/ScreenZone base palette for the zone.
		// Any other value is a Level or ScreenZone tag group solved on top of the base.
		//
		std::string tag;
		//
		// Whether all participants for this tag fit within the remaining pen budget
		//
		bool fit = false;
		//
		// Number of unique colors in the union palette before the cap was applied.
		// Equal to pensUsed when there is no overflow.
		//
		int unionSize = 0;
		//
		// Number of colors that were dropped by the cap pass (unionSize - pensAvailable, >= 0)
		//
		int overflow = 0;
		//
		// Number of pens consumed by the participants in this tag group (excluding base)
		//
		int pensConsumed = 0;
		//
		// Number of pens used in total by the time this solution is complete
		// (always pens + zone pens + these tag pens)
		//
		int pensUsed = 0;
		//
		// Target system and mode strings — needed by the UI to recreate a display converter
		//
		std::string targetSystem;
		std::string targetMode;
		//
		// Pen slot assignments produced by the solver.
		// Index = pen number, value = system color index (-1 = free/unassigned).
		// Used by the UI to create a fresh display converter on demand.
		//
		std::vector<int> occupiedSlots;
		//
		// Per-participant results for the participants that were solved in this pass
		//
		std::vector<PaletteParticipantResult> participantResults;
		//
		// Live converters produced during this solve pass, keyed by participant index.
		// Each converter holds the source image, the solved palette, and the converted
		// preview image. The UI uses these directly instead of maintaining its own copies.
		//
		std::unordered_map<int, std::shared_ptr<IBitmapConverter>> converters;
		//
		// GFXParams used for each converter (palette locked to the solution slots)
		//
		std::unordered_map<int, GFXParams> converterGfx;
		//
		// Per-overflow-color remap decisions made during the cap step for this solution.
		// One entry for each color in the union that exceeded the pen budget.
		// Transient — not serialised.
		//
		struct OverflowRemap {
			//
			// The color that could not fit in the palette
			//
			int overflowColorIndex = -1;
			uint8_t overflowR = 0, overflowG = 0, overflowB = 0;
			//
			// The accepted slot this overflow color was nearest to, and its color before
			// any remap was applied
			//
			int slot = -1;
			int nearestColorIndex = -1;
			uint8_t nearestR = 0, nearestG = 0, nearestB = 0;
			//
			// The color the slot was changed to after blending (equals nearestColorIndex for
			// HardCap where the slot is unchanged and the overflow color is simply dropped)
			//
			int resultColorIndex = -1;
			uint8_t resultR = 0, resultG = 0, resultB = 0;
			//
			// True when the overflow color was dropped without modifying any accepted slot
			// (HardCap behaviour)
			//
			bool dropped = false;
		};
		std::vector<OverflowRemap> overflowRemaps;
	};

	//
	// Solve result for a single palette zone.
	// Contains the final solved palette converter (on success) and per-participant details.
	//
	struct PaletteZoneSolution {
		//
		// Index into PaletteParams::zones this solution corresponds to
		//
		int zoneIndex = -1;
		//
		// Whether all Always-role participants fit without overflow
		// (Level / ScreenZone participants are solved per-group separately)
		//
		bool baseFit = false;
		//
		// Total pen slots used by Always participants across all zones (global Always base)
		//
		int basePensUsed = 0;
		//
		// Maximum pen slots available for this zone (from IPaletteConverter::PaletteMaxColors)
		//
		int pensAvailable = 0;
		//
		// The solved palette converter for this zone after all Always participants have been applied.
		// nullptr if the solve was not attempted or the target system/mode is invalid.
		//
		std::shared_ptr<IPaletteConverter> solvedPalette;
		//
		// Per-participant results, one entry per participant in PaletteZone::participants
		//
		std::vector<PaletteParticipantResult> participantResults;
		//
		// Per-tag group solve results: key is the tag string, value lists the participant results
		// for that group (Always participants are in the "" key)
		//
		std::vector<std::pair<std::string, std::vector<PaletteParticipantResult>>> tagGroupResults;
		//
		// Ordered list of tag solutions for this zone:
		// index 0 is always the base (Always + Zone/ScreenZone combined, tag == "")
		// subsequent entries are one per Level / ScreenZone tag, built on top of the base
		//
		std::vector<PaletteTagSolution> tagSolutions;
	};

	//
	// Full solve result for one PaletteParams, containing one PaletteZoneSolution per zone
	//
	struct PaletteSolution {
		//
		// Whether the solve completed without any hard errors
		// (false if the target system/mode is unknown or a converter could not be created)
		//
		bool valid = false;
		//
		// Human-readable summary message
		//
		std::string summary;
		//
		// One solution entry per zone, in the same order as PaletteParams::zones
		//
		std::vector<PaletteZoneSolution> zones;
	};

	//
	// Solves a PaletteParams configuration by loading source images, running conversions
	// and checking that all participant colors fit within the zone pen budget.
	//
	// The solver operates in three passes:
	//   Pass 1 (global Always): Always participants from ALL zones are quantized together
	//                           into a single base palette so the same pen slots hold the
	//                           same colors in every zone and every level.
	//   Pass 2 (zone base): per zone, ZoneAlways participants are fitted on top of
	//                       the global Always pens. The resulting palette is the zone base
	//                       that is stable across all levels within that zone.
	//   Pass 3 (level tags): per zone per level-tag, Level participants are fitted on top
	//                        of that zone's base palette. Each combination produces one
	//                        PaletteTagSolution stored in PaletteZoneSolution::tagSolutions.
	//
	// The solver does not modify the project. All palette manipulation is done on
	// temporary converter instances created via Converters::GetBitmapConverter.
	//
	class PaletteSolver {
	public:
		//
		// Run the full solve for the given palette params.
		// projectFolder: absolute path to the project folder, used to resolve relative source paths.
		// Returns a PaletteSolution describing the outcome for every zone.
		//
		static PaletteSolution Solve(const PaletteParams* params, const std::string& projectFolder);
		//
		// Write the solved palette assignments from a PaletteSolution back into each
		// participant's stored project GFXParams (PaletteLocked, PaletteEnabled, PaletteColors).
		// Safe to call from automated tools that use only the lib.
		// Calls Project::MarkAsModified() on success (at least one item updated).
		// Does nothing if the solution is invalid or params is null.
		//
		static void Validate(const PaletteSolution& solution, const PaletteParams* params);
	};
} // namespace RetrodevLib
