// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Palette asset -- hardware pen assignment and colour lookup.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#include <assets/palette/palette.h>
#include <convert/converters.h>
#include <assets/image/image.h>
#include <project/project.h>
#include <convert/convert.bitmap.params.h>
#include <algorithm>
#include <map>

namespace RetrodevLib {
	//
	// Helper: build a GFXParams with only the system/mode fields populated.
	// All other fields keep their defaults so conversion uses the item's own settings.
	//
	static GFXParams MakeGfxParams(const std::string& targetSystem, const std::string& targetMode, const std::string& targetPaletteType = "") {
		GFXParams gfx;
		gfx.SParams.TargetSystem = targetSystem;
		gfx.SParams.TargetMode = targetMode;
		gfx.SParams.PaletteType = targetPaletteType;
		return gfx;
	}
	//
	// Helper: resolve the absolute source path for a build item.
	// Tries Bitmap, Tileset and Sprite project entries in order matching the participant type.
	// Returns empty string if the item is not found.
	//
	static std::string ResolveSourcePath(const PaletteParticipant& participant, const std::string& projectFolder) {
		std::string rel;
		if (participant.buildItemType == "Bitmap")
			rel = Project::BitmapGetSourcePath(participant.buildItemName);
		else if (participant.buildItemType == "Tilemap")
			rel = Project::TilesetGetSourcePath(participant.buildItemName);
		else if (participant.buildItemType == "Sprite")
			rel = Project::SpriteGetSourcePath(participant.buildItemName);
		if (rel.empty())
			return "";
		//
		// If the path is already absolute leave it as-is, otherwise prepend the project folder
		//
		if (rel.size() > 1 && (rel[1] == ':' || rel[0] == '/'))
			return rel;
		return projectFolder + "/" + rel;
	}
	//
	// Helper: get the GFXParams stored for a build item (controls quantization, dithering, etc.).
	// Falls back to a minimal params block when the item is not found.
	//
	static GFXParams GetItemGfxParams(const PaletteParticipant& participant, const std::string& targetSystem, const std::string& targetMode) {
		GFXParams* stored = nullptr;
		if (participant.buildItemType == "Bitmap")
			Project::BitmapGetCfg(participant.buildItemName, &stored);
		else if (participant.buildItemType == "Tilemap")
			Project::TilesetGetCfg(participant.buildItemName, &stored);
		else if (participant.buildItemType == "Sprite")
			Project::SpriteGetCfg(participant.buildItemName, &stored);
		if (stored)
			return *stored;
		return MakeGfxParams(targetSystem, targetMode);
	}
	//
	// Collect the unique system color indices used by one participant.
	// Runs Convert() with no forced locks (only user-specified locks from the item's own GFXParams)
	// so the quantizer freely picks colors from the full system palette.
	// Returns the list of assigned unique color indices, ordered by pen slot.
	// outConverter / outGfx receive the live converter and the params used (for UI use).
	// On failure (missing item / bad image) returns an empty list and sets result.status.
	//
	static std::vector<int> CollectParticipantColors(int participantIndex, const PaletteParticipant& participant, const std::string& projectFolder, const std::string& targetSystem,
													 const std::string& targetMode, const std::string& targetPaletteType, std::shared_ptr<IBitmapConverter>* outConverter,
													 GFXParams* outGfx, PaletteParticipantResult& result) {
		result.participantIndex = participantIndex;
		//
		// Resolve and load the source image
		//
		std::string sourcePath = ResolveSourcePath(participant, projectFolder);
		if (sourcePath.empty()) {
			result.status = PaletteParticipantStatus::Missing;
			result.message = "Build item \"" + participant.buildItemName + "\" (" + participant.buildItemType + ") not found in project.";
			return {};
		}
		auto image = Image::ImageLoad(sourcePath);
		if (!image) {
			result.status = PaletteParticipantStatus::ImageLoadFailed;
			result.message = "Could not load source image: " + sourcePath;
			return {};
		}
		//
		// Build GFXParams from the item's own stored settings; override the palette type.
		// Do NOT seed any occupied slots -- quantizer runs freely to find this image's best colors.
		// UseSourcePalette is suppressed: the solver does its own free quantization.
		// Clear all lock/enable/color arrays now, before GetBitmapConverter reads them,
		// so a previous Validate() pass cannot leave pens locked and block re-collection.
		//
		GFXParams gfx = GetItemGfxParams(participant, targetSystem, targetMode);
		gfx.SParams.PaletteType = targetPaletteType;
		gfx.QParams.UseSourcePalette = false;
		gfx.SParams.PaletteLocked.clear();
		gfx.SParams.PaletteEnabled.clear();
		gfx.SParams.PaletteColors.clear();
		//
		// Unlock and enable all pens so the quantizer can assign any color freely
		//
		auto converter = Converters::GetBitmapConverter(&gfx);
		if (!converter) {
			result.status = PaletteParticipantStatus::Missing;
			result.message = "No converter available for target system/mode.";
			return {};
		}
		int maxPens = converter->GetPalette()->PaletteMaxColors();
		gfx.SParams.PaletteLocked.assign(maxPens, false);
		gfx.SParams.PaletteEnabled.assign(maxPens, true);
		gfx.SParams.PaletteColors.assign(maxPens, -1);
		//
		// If this asset has a raw-data transparent pen, disable that pen slot so the
		// quantizer never assigns any color to it. The slot is reserved exclusively
		// for the export-time transparency key; no image color should ever land there.
		//
		int transparentPenSlot = gfx.RParams.TransparentPen;
		if (transparentPenSlot >= 0 && transparentPenSlot < maxPens)
			gfx.SParams.PaletteEnabled[transparentPenSlot] = false;
		if (gfx.RParams.TargetWidth == 0 || gfx.RParams.TargetHeight == 0) {
			gfx.RParams.TargetWidth = 1;
			gfx.RParams.TargetHeight = 1;
		}
		converter->SetOriginal(image);
		converter->Convert(&gfx);
		//
		// Collect unique assigned color indices in pen order.
		// The transparent pen slot was disabled above so the quantizer will never have
		// assigned a color to it; the explicit skip here is a safety guard only.
		//
		auto palette = converter->GetPalette();
		int transparentPen = gfx.RParams.TransparentPen;
		std::vector<int> colors;
		for (int i = 0; i < maxPens; i++) {
			if (i == transparentPen)
				continue;
			if (palette->PenGetUsed(i))
				colors.push_back(palette->PenGetColorIndex(i));
		}
		result.status = PaletteParticipantStatus::OK;
		result.message = "Colors collected (" + std::to_string(colors.size()) + " pen(s)).";
		if (outConverter)
			*outConverter = converter;
		if (outGfx)
			*outGfx = gfx;
		return colors;
	}
	//
	// Run the final conversion for one participant against a fixed palette (all pens locked).
	// The quantizer remaps every pixel to the nearest color in the fixed palette.
	// outConverter / outGfx receive the resulting live converter.
	// Returns the PaletteParticipantResult for this participant.
	//
	static PaletteParticipantResult FinalConvertParticipant(int participantIndex, const PaletteParticipant& participant, const std::string& projectFolder,
															const std::string& targetSystem, const std::string& targetMode, const std::string& targetPaletteType,
															const std::vector<int>& fixedPalette, PaletteOverflowMethod overflowMethod,
															std::shared_ptr<IBitmapConverter>* outConverter, GFXParams* outGfx) {
		PaletteParticipantResult result;
		result.participantIndex = participantIndex;
		//
		// Resolve and load the source image
		//
		std::string sourcePath = ResolveSourcePath(participant, projectFolder);
		if (sourcePath.empty()) {
			result.status = PaletteParticipantStatus::Missing;
			result.message = "Build item \"" + participant.buildItemName + "\" (" + participant.buildItemType + ") not found in project.";
			return result;
		}
		auto image = Image::ImageLoad(sourcePath);
		if (!image) {
			result.status = PaletteParticipantStatus::ImageLoadFailed;
			result.message = "Could not load source image: " + sourcePath;
			return result;
		}
		//
		// Build GFXParams and seed all pens from the fixed palette (all locked).
		// Pens beyond fixedPalette.size() are disabled so the quantizer ignores them.
		// UseSourcePalette is suppressed: the solver maps to the fixed palette directly.
		//
		GFXParams gfx = GetItemGfxParams(participant, targetSystem, targetMode);
		gfx.SParams.PaletteType = targetPaletteType;
		gfx.QParams.UseSourcePalette = false;
		auto converter = Converters::GetBitmapConverter(&gfx);
		if (!converter) {
			result.status = PaletteParticipantStatus::Missing;
			result.message = "No converter available for target system/mode.";
			return result;
		}
		int maxPens = converter->GetPalette()->PaletteMaxColors();
		gfx.SParams.PaletteLocked.assign(maxPens, false);
		gfx.SParams.PaletteEnabled.assign(maxPens, false);
		gfx.SParams.PaletteColors.assign(maxPens, -1);
		for (int i = 0; i < (int)fixedPalette.size() && i < maxPens; i++) {
			gfx.SParams.PaletteColors[i] = fixedPalette[i];
			gfx.SParams.PaletteLocked[i] = true;
			gfx.SParams.PaletteEnabled[i] = true;
		}
		//
		// If this asset has a raw-data transparent pen, disable that slot so the
		// remapping pass never maps any pixel to it. The fixed palette color that
		// other assets placed in that slot is irrelevant for this asset.
		//
		int transparentPenSlot = gfx.RParams.TransparentPen;
		if (transparentPenSlot >= 0 && transparentPenSlot < maxPens) {
			gfx.SParams.PaletteEnabled[transparentPenSlot] = false;
			gfx.SParams.PaletteLocked[transparentPenSlot] = false;
			gfx.SParams.PaletteColors[transparentPenSlot] = -1;
		}
		if (gfx.RParams.TargetWidth == 0 || gfx.RParams.TargetHeight == 0) {
			gfx.RParams.TargetWidth = 1;
			gfx.RParams.TargetHeight = 1;
		}
		converter->SetOriginal(image);
		//
		// Assign to stable output storage first, then Convert exactly once against it.
		// Convert() stores raw pointers into the GFXParams vectors internally; copying
		// GFXParams after Convert() would dangle those pointers and corrupt the vtable.
		//
		if (outConverter && outGfx) {
			*outConverter = converter;
			*outGfx = gfx;
			(*outConverter)->Convert(outGfx);
		} else {
			converter->Convert(&gfx);
		}
		//
		// Read back the live palette to build per-pen detail and a rich result message.
		// Use the outGfx converter when available (stable storage), otherwise use the local one.
		//
		{
			auto livePalette = outConverter ? (*outConverter)->GetPalette() : converter->GetPalette();
			result.overflowMethodApplied = overflowMethod;
			int usedPens = 0;
			for (int i = 0; i < (int)fixedPalette.size(); i++) {
				if (!livePalette->PenGetEnabled(i))
					continue;
				PaletteParticipantResult::ColorSwatch info;
				info.slot = i;
				info.colorIndex = livePalette->PenGetColorIndex(i);
				RgbColor rgb = livePalette->GetSystemColorByIndex(info.colorIndex);
				info.r = rgb.r;
				info.g = rgb.g;
				info.b = rgb.b;
				result.assignedColors.push_back(info);
				usedPens++;
			}
			//
			// Build the message: method name + pen count, plus transparent pen note when applicable
			//
				static const char* kMethodNames[] = {"Hard Cap", "Soft Cap", "Weighted Blend", "Median"};
				const char* methodName = kMethodNames[(int)overflowMethod];
				result.message = std::string(methodName) + "  \xc2\xb7  " + std::to_string(usedPens) + " pen(s) assigned";
				if (gfx.RParams.UseTransparentColor && transparentPenSlot >= 0)
					result.message += "  \xc2\xb7  +1 transparent (pen " + std::to_string(transparentPenSlot) + ")";
			}
		result.status = PaletteParticipantStatus::OK;
		return result;
	}
	//
	// Build the union palette from per-participant color lists in priority order.
	// colorsPerParticipant: each entry is the ordered list of color indices for one participant,
	//                       listed in priority order (Always first, then Zone, then Level).
	// Returns a deduplicated list of system color indices preserving the first-seen order.
	//
	static std::vector<int> BuildUnionPalette(const std::vector<std::vector<int>>& colorsPerParticipant) {
		std::vector<int> unionPalette;
		std::vector<bool> seen;
		//
		// Determine the maximum color index to size the seen table
		//
		int maxIdx = 0;
		for (const auto& cv : colorsPerParticipant)
			for (int c : cv)
				if (c > maxIdx)
					maxIdx = c;
		seen.assign(maxIdx + 1, false);
		for (const auto& cv : colorsPerParticipant) {
			for (int c : cv) {
				if (!seen[c]) {
					seen[c] = true;
					unionPalette.push_back(c);
				}
			}
		}
		return unionPalette;
	}
	//
	// After CapPalette assigns colors to pen slots, remap the fixed palette so that each
	// transparent pen slot holds a color that is NOT needed by any transparency-using
	// participant.  This prevents those participants from losing access to a color just
	// because it happens to sit in a pen slot they cannot use.
	//
	// fixedPalette         : in/out pen-slot-indexed color list (fixedPalette[penSlot] = colorIdx)
	// transparentSlots     : set of pen slots used as transparent by at least one participant
	// neededByTransparents : set of color indices that transparency-using participants need
	// lockedSlots          : set of pen slots that must not be moved (e.g. preloaded pins)
	//
	// For every transparent slot whose current color IS needed by transparency-using
	// participants the function looks for a non-transparent, non-locked slot whose color
	// is NOT needed by them and swaps the two.  If no such swap candidate exists the slot
	// is left as-is (the situation is unresolvable without adding more pens).
	//
	static void RemapTransparentPenSlots(std::vector<int>& fixedPalette,
										 const std::vector<int>& transparentSlots,
										 const std::vector<bool>& neededByTransparents,
										 const std::vector<bool>& lockedSlots = {}) {
		if (transparentSlots.empty())
			return;
		//
		// Build quick lookups for transparent and locked slots
		//
		std::vector<bool> isTransparentSlot(fixedPalette.size(), false);
		for (int t : transparentSlots)
			if (t >= 0 && t < (int)fixedPalette.size())
				isTransparentSlot[t] = true;
		//
		// For each transparent slot: if its color is needed by transparency-using participants,
		// swap it with the first non-transparent, non-locked slot whose color is not needed.
		// Locked slots are never moved -- they hold preloaded user-pinned colors.
		//
		for (int t : transparentSlots) {
			if (t < 0 || t >= (int)fixedPalette.size())
				continue;
			//
			// Never touch a locked (preloaded) slot even if it is also a transparent slot
			//
			if (t < (int)lockedSlots.size() && lockedSlots[t])
				continue;
			int colorAtSlot = fixedPalette[t];
			bool needed = (colorAtSlot >= 0 && colorAtSlot < (int)neededByTransparents.size() && neededByTransparents[colorAtSlot]);
			if (!needed)
				continue;
			//
			// Find a non-transparent, non-locked slot whose color is not needed
			//
			for (int s = 0; s < (int)fixedPalette.size(); s++) {
				if (isTransparentSlot[s])
					continue;
				if (s < (int)lockedSlots.size() && lockedSlots[s])
					continue;
				int colorAtS = fixedPalette[s];
				bool sNeeded = (colorAtS >= 0 && colorAtS < (int)neededByTransparents.size() && neededByTransparents[colorAtS]);
				if (!sNeeded) {
					//
					// Swap: the transparent slot now holds a disposable color; the needed
					// color moves to a slot that transparency-using participants can access.
					//
					fixedPalette[t] = colorAtS;
					fixedPalette[s] = colorAtSlot;
					break;
				}
			}
		}
	}
	//
	// Apply the cap method to reduce unionPalette to at most pensAvailable entries.
	// palette: system palette converter used for RGB<->index lookups (required for all methods
	//          except HardCap; may be nullptr, which forces a fallback to HardCap).
	// outRemaps: optional; receives one OverflowRemap entry per color that exceeded the budget.
	// Returns the capped palette and sets overflow to the number of dropped colors.
	//
	static std::vector<int> CapPalette(const std::vector<int>& unionPalette, int pensAvailable, PaletteOverflowMethod method, IPaletteConverter* palette, int& overflow,
									   std::vector<PaletteTagSolution::OverflowRemap>* outRemaps = nullptr) {
		if ((int)unionPalette.size() <= pensAvailable) {
			overflow = 0;
			return unionPalette;
		}
		overflow = (int)unionPalette.size() - pensAvailable;
		//
		// HardCap: keep the first pensAvailable entries (highest-priority colors survive).
		// Overflow colors are dropped without modifying any accepted slot.
		//
		if (method == PaletteOverflowMethod::HardCap || !palette) {
			if (outRemaps) {
				for (int i = pensAvailable; i < (int)unionPalette.size(); i++) {
					PaletteTagSolution::OverflowRemap rm;
					rm.overflowColorIndex = unionPalette[i];
					RgbColor ovRgb = palette ? palette->GetSystemColorByIndex(unionPalette[i]) : RgbColor{};
					rm.overflowR = ovRgb.r;
					rm.overflowG = ovRgb.g;
					rm.overflowB = ovRgb.b;
					//
					// Find nearest accepted slot just for informational display, but don't modify it
					//
					rm.slot = -1;
					if (palette && pensAvailable > 0) {
						int bestSlot = 0;
						int bestDist = palette->ColorDistance(ovRgb, palette->GetSystemColorByIndex(unionPalette[0]));
						for (int s = 1; s < pensAvailable; s++) {
							int d = palette->ColorDistance(ovRgb, palette->GetSystemColorByIndex(unionPalette[s]));
							if (d < bestDist) {
								bestDist = d;
								bestSlot = s;
							}
						}
						rm.slot = bestSlot;
						rm.nearestColorIndex = unionPalette[bestSlot];
						RgbColor nearRgb = palette->GetSystemColorByIndex(unionPalette[bestSlot]);
						rm.nearestR = nearRgb.r;
						rm.nearestG = nearRgb.g;
						rm.nearestB = nearRgb.b;
					}
					rm.resultColorIndex = rm.nearestColorIndex;
					rm.resultR = rm.nearestR;
					rm.resultG = rm.nearestG;
					rm.resultB = rm.nearestB;
					rm.dropped = true;
					outRemaps->push_back(rm);
				}
			}
			return std::vector<int>(unionPalette.begin(), unionPalette.begin() + pensAvailable);
		}
		//
		// Build initial accepted set (highest-priority entries up to the pen budget)
		//
		std::vector<int> accepted(unionPalette.begin(), unionPalette.begin() + pensAvailable);
		//
		// SoftCap: for each overflow color find its nearest accepted match and replace that
		// match with the system color nearest to the 50/50 RGB midpoint of the two.
		//
		if (method == PaletteOverflowMethod::SoftCap) {
			for (int i = pensAvailable; i < (int)unionPalette.size(); i++) {
				RgbColor ovRgb = palette->GetSystemColorByIndex(unionPalette[i]);
				int bestSlot = 0;
				int bestDist = palette->ColorDistance(ovRgb, palette->GetSystemColorByIndex(accepted[0]));
				for (int s = 1; s < (int)accepted.size(); s++) {
					int d = palette->ColorDistance(ovRgb, palette->GetSystemColorByIndex(accepted[s]));
					if (d < bestDist) {
						bestDist = d;
						bestSlot = s;
					}
				}
				RgbColor accRgb = palette->GetSystemColorByIndex(accepted[bestSlot]);
				RgbColor mid((uint8_t)((accRgb.r + ovRgb.r) / 2), (uint8_t)((accRgb.g + ovRgb.g) / 2), (uint8_t)((accRgb.b + ovRgb.b) / 2));
				int resultIdx = palette->GetSystemIndexByColor(mid, "");
				if (outRemaps) {
					PaletteTagSolution::OverflowRemap rm;
					rm.overflowColorIndex = unionPalette[i];
					rm.overflowR = ovRgb.r;
					rm.overflowG = ovRgb.g;
					rm.overflowB = ovRgb.b;
					rm.slot = bestSlot;
					rm.nearestColorIndex = accepted[bestSlot];
					rm.nearestR = accRgb.r;
					rm.nearestG = accRgb.g;
					rm.nearestB = accRgb.b;
					RgbColor resultRgb = palette->GetSystemColorByIndex(resultIdx);
					rm.resultColorIndex = resultIdx;
					rm.resultR = resultRgb.r;
					rm.resultG = resultRgb.g;
					rm.resultB = resultRgb.b;
					rm.dropped = false;
					outRemaps->push_back(rm);
				}
				accepted[bestSlot] = resultIdx;
			}
			return accepted;
		}
		//
		// WeightedBlend: like SoftCap but 67% accepted + 33% overflow -- preserves dominant
		// colors better while still pulling toward overflow neighbors.
		//
		if (method == PaletteOverflowMethod::WeightedBlend) {
			for (int i = pensAvailable; i < (int)unionPalette.size(); i++) {
				RgbColor ovRgb = palette->GetSystemColorByIndex(unionPalette[i]);
				int bestSlot = 0;
				int bestDist = palette->ColorDistance(ovRgb, palette->GetSystemColorByIndex(accepted[0]));
				for (int s = 1; s < (int)accepted.size(); s++) {
					int d = palette->ColorDistance(ovRgb, palette->GetSystemColorByIndex(accepted[s]));
					if (d < bestDist) {
						bestDist = d;
						bestSlot = s;
					}
				}
				RgbColor accRgb = palette->GetSystemColorByIndex(accepted[bestSlot]);
				RgbColor blended((uint8_t)((accRgb.r * 2 + ovRgb.r) / 3), (uint8_t)((accRgb.g * 2 + ovRgb.g) / 3), (uint8_t)((accRgb.b * 2 + ovRgb.b) / 3));
				int resultIdx = palette->GetSystemIndexByColor(blended, "");
				if (outRemaps) {
					PaletteTagSolution::OverflowRemap rm;
					rm.overflowColorIndex = unionPalette[i];
					rm.overflowR = ovRgb.r;
					rm.overflowG = ovRgb.g;
					rm.overflowB = ovRgb.b;
					rm.slot = bestSlot;
					rm.nearestColorIndex = accepted[bestSlot];
					rm.nearestR = accRgb.r;
					rm.nearestG = accRgb.g;
					rm.nearestB = accRgb.b;
					RgbColor resultRgb = palette->GetSystemColorByIndex(resultIdx);
					rm.resultColorIndex = resultIdx;
					rm.resultR = resultRgb.r;
					rm.resultG = resultRgb.g;
					rm.resultB = resultRgb.b;
					rm.dropped = false;
					outRemaps->push_back(rm);
				}
				accepted[bestSlot] = resultIdx;
			}
			return accepted;
		}
		//
		// Median: cluster overflow colors with their nearest accepted entry and replace each
		// accepted entry with the RGB centroid of the full cluster (accepted + all its overflow
		// neighbors). Multiple overflow colors hitting the same entry are absorbed equally.
		//
		if (method == PaletteOverflowMethod::Median) {
			//
			// Accumulate per-slot RGB sums from overflow colors only.
			// count[s] == 0 means no overflow color was mapped to slot s, so its accepted
			// color is kept unchanged. count[s] >= 1 means at least one overflow color hit
			// this slot; the centroid of those overflow colors replaces the accepted entry.
			// This gives Median a meaningful different result from HardCap: the slot shifts
			// toward the overflow colors that were most similar to it, rather than staying put.
			//
			std::vector<int> sumR(accepted.size(), 0), sumG(accepted.size(), 0), sumB(accepted.size(), 0);
			std::vector<int> count(accepted.size(), 0);
			//
			// Track which accepted slot each overflow color maps to (for remap recording)
			//
			std::vector<int> overflowSlots;
			for (int i = pensAvailable; i < (int)unionPalette.size(); i++) {
				RgbColor ovRgb = palette->GetSystemColorByIndex(unionPalette[i]);
				int bestSlot = 0;
				int bestDist = palette->ColorDistance(ovRgb, palette->GetSystemColorByIndex(accepted[0]));
				for (int s = 1; s < (int)accepted.size(); s++) {
					int d = palette->ColorDistance(ovRgb, palette->GetSystemColorByIndex(accepted[s]));
					if (d < bestDist) {
						bestDist = d;
						bestSlot = s;
					}
				}
				overflowSlots.push_back(bestSlot);
				sumR[bestSlot] += ovRgb.r;
				sumG[bestSlot] += ovRgb.g;
				sumB[bestSlot] += ovRgb.b;
				count[bestSlot]++;
			}
			//
			// Snapshot accepted colors before mutation so remap records show the original nearest
			//
			std::vector<int> acceptedBefore = accepted;
			for (int s = 0; s < (int)accepted.size(); s++) {
				if (count[s] > 0) {
					//
					// Replace accepted entry with the system color nearest to the centroid
					// of all overflow colors that clustered here, blended 50/50 with the
					// original accepted color so the slot doesn't drift too far from its
					// original priority position.
					//
					RgbColor accRgb = palette->GetSystemColorByIndex(accepted[s]);
					RgbColor centroid((uint8_t)((sumR[s] / count[s] + accRgb.r) / 2), (uint8_t)((sumG[s] / count[s] + accRgb.g) / 2),
									  (uint8_t)((sumB[s] / count[s] + accRgb.b) / 2));
					accepted[s] = palette->GetSystemIndexByColor(centroid, "");
				}
			}
			if (outRemaps) {
				for (int i = 0; i < (int)overflowSlots.size(); i++) {
					int ovIdx = pensAvailable + i;
					int slot = overflowSlots[i];
					RgbColor ovRgb = palette->GetSystemColorByIndex(unionPalette[ovIdx]);
					RgbColor nearRgb = palette->GetSystemColorByIndex(acceptedBefore[slot]);
					RgbColor resultRgb = palette->GetSystemColorByIndex(accepted[slot]);
					PaletteTagSolution::OverflowRemap rm;
					rm.overflowColorIndex = unionPalette[ovIdx];
					rm.overflowR = ovRgb.r;
					rm.overflowG = ovRgb.g;
					rm.overflowB = ovRgb.b;
					rm.slot = slot;
					rm.nearestColorIndex = acceptedBefore[slot];
					rm.nearestR = nearRgb.r;
					rm.nearestG = nearRgb.g;
					rm.nearestB = nearRgb.b;
					rm.resultColorIndex = accepted[slot];
					rm.resultR = resultRgb.r;
					rm.resultG = resultRgb.g;
					rm.resultB = resultRgb.b;
					rm.dropped = false;
					outRemaps->push_back(rm);
				}
			}
			return accepted;
		}
		//
		// Fallback to HardCap for any unrecognised method value
		//
		return std::vector<int>(unionPalette.begin(), unionPalette.begin() + pensAvailable);
	}
	//
	// Helper: take a snapshot into a PaletteTagSolution for UI display.
	//
	static PaletteTagSolution SnapshotTagSolution(const std::string& tag, const std::vector<int>& fixedPalette, const std::string& targetSystem, const std::string& targetMode,
												  int pensAvailable, bool fit, int unionSize, int overflow, const std::vector<PaletteParticipantResult>& results,
												  std::vector<PaletteTagSolution::OverflowRemap> remaps = {}) {
		PaletteTagSolution ts;
		ts.tag = tag;
		ts.fit = fit;
		ts.pensUsed = (int)fixedPalette.size();
		ts.pensConsumed = (int)fixedPalette.size();
		ts.unionSize = unionSize;
		ts.overflow = overflow;
		ts.targetSystem = targetSystem;
		ts.targetMode = targetMode;
		ts.participantResults = results;
		ts.overflowRemaps = std::move(remaps);
		//
		// Convert the fixed palette back into occupiedSlots (pen i -> fixedPalette[i], rest -1)
		//
		ts.occupiedSlots.assign(pensAvailable, -1);
		for (int i = 0; i < (int)fixedPalette.size() && i < pensAvailable; i++)
			ts.occupiedSlots[i] = fixedPalette[i];
		return ts;
	}
	//
	// Solve one zone given a list of Always color lists already collected globally.
	// alwaysColors: union-capped palette from the global Always pass (shared across all zones/levels).
	// Pass 2: collect ZoneAlways participant colors, merge with alwaysColors, cap, final-convert.
	// Pass 3: per level-tag, collect Level participant colors, merge on top of zone base, cap, final-convert.
	//
	static PaletteZoneSolution SolveZone(int zoneIndex, const PaletteZone& zone, const std::string& targetSystem, const std::string& targetPaletteType,
									 const std::string& projectFolder, PaletteOverflowMethod overflowMethod, const std::vector<int>& alwaysColors,
									 std::vector<PaletteTagSolution::OverflowRemap> alwaysRemaps = {},
									 const std::vector<bool>& lockedSlots = {}) {
		const std::string& targetMode = zone.targetMode;
		PaletteZoneSolution zoneSolution;
		zoneSolution.zoneIndex = zoneIndex;
		zoneSolution.participantResults.resize(zone.participants.size());
		//
		// Probe PaletteMaxColors by creating a one-shot primed converter.
		// The probe converter is kept alive for the duration of SolveZone so its palette
		// can be passed to CapPalette for RGB-based overflow methods.
		//
		std::shared_ptr<IBitmapConverter> probeConverter;
		{
			GFXParams probeGfx = MakeGfxParams(targetSystem, targetMode, targetPaletteType);
			probeConverter = Converters::GetBitmapConverter(&probeGfx);
			if (!probeConverter) {
				zoneSolution.baseFit = false;
				return zoneSolution;
			}
			auto primeDummy = Image::ImageCreate(1, 1);
			if (primeDummy) {
				probeGfx.RParams.TargetWidth = 1;
				probeGfx.RParams.TargetHeight = 1;
				probeConverter->SetOriginal(primeDummy);
				probeConverter->Convert(&probeGfx);
			}
			zoneSolution.pensAvailable = probeConverter->GetPalette()->PaletteMaxColors();
		}
		IPaletteConverter* zonePalette = probeConverter->GetPalette().get();
		//
		// Helper struct for capturing converters produced during the zone solve passes
		//
		struct ConverterEntry {
			std::shared_ptr<IBitmapConverter> conv;
			GFXParams gfx;
		};
		//
		// Collect the color lists for every ZoneAlways participant (Pass 2: zone base)
		// and build the zone union palette on top of alwaysColors.
		//
		std::vector<int> zoneUnion = alwaysColors;
		{
			std::vector<bool> seenZone;
			int maxIdx = 0;
			for (int c : zoneUnion)
				if (c > maxIdx)
					maxIdx = c;
			seenZone.assign(maxIdx + 1, false);
			for (int c : zoneUnion)
				seenZone[c] = true;
			for (int pi = 0; pi < (int)zone.participants.size(); pi++) {
				const PaletteParticipant& p = zone.participants[pi];
				if (p.role != PaletteParticipantRole::ZoneAlways)
					continue;
				PaletteParticipantResult dummy;
				std::vector<int> colors = CollectParticipantColors(pi, p, projectFolder, targetSystem, targetMode, targetPaletteType, nullptr, nullptr, dummy);
				for (int c : colors) {
					if (c >= (int)seenZone.size()) {
						seenZone.resize(c + 1, false);
					}
					if (!seenZone[c]) {
						seenZone[c] = true;
						zoneUnion.push_back(c);
					}
				}
			}
		}
		int zoneOverflow = 0;
		std::vector<PaletteTagSolution::OverflowRemap> zoneRemaps;
		std::vector<int> zoneFixed = CapPalette(zoneUnion, zoneSolution.pensAvailable, overflowMethod, zonePalette, zoneOverflow, &zoneRemaps);
		//
		// Remap transparent pen slots: for each participant that has a TransparentPen,
		// collect its needed colors and ensure the transparent slot holds a color that
		// those participants do NOT need, so they lose no accessible colors.
		//
		{
			std::vector<int> transSlotsZone;
			std::vector<bool> neededZone;
			for (int pi = 0; pi < (int)zone.participants.size(); pi++) {
				const PaletteParticipant& p = zone.participants[pi];
				GFXParams pg = GetItemGfxParams(p, targetSystem, targetMode);
				int ts = pg.RParams.TransparentPen;
				if (ts < 0)
					continue;
				bool alreadyInList = false;
				for (int v : transSlotsZone)
					if (v == ts) { alreadyInList = true; break; }
				if (!alreadyInList)
					transSlotsZone.push_back(ts);
				PaletteParticipantResult dummy2;
				std::vector<int> pc = CollectParticipantColors(pi, p, projectFolder, targetSystem, targetMode, targetPaletteType, nullptr, nullptr, dummy2);
				for (int c : pc) {
					if (c >= (int)neededZone.size())
						neededZone.resize(c + 1, false);
					neededZone[c] = true;
				}
			}
			RemapTransparentPenSlots(zoneFixed, transSlotsZone, neededZone, lockedSlots);
		}
		//
		// Merge always-pass remaps into zone remaps so the base tag solution shows the full picture
		//
		for (auto& rm : alwaysRemaps)
			zoneRemaps.insert(zoneRemaps.begin(), rm);
		zoneSolution.baseFit = (zoneOverflow == 0);
		//
		// Final-convert all Always + ZoneAlways participants against the zone fixed palette
		// and build the base tag solution
		//
		std::vector<PaletteParticipantResult> zoneBaseResults;
		std::unordered_map<int, ConverterEntry> zoneBaseConverters;
		for (int pi = 0; pi < (int)zone.participants.size(); pi++) {
			const PaletteParticipant& p = zone.participants[pi];
			if (p.role == PaletteParticipantRole::Level) {
				PaletteParticipantResult skipped;
				skipped.participantIndex = pi;
				skipped.status = PaletteParticipantStatus::Skipped;
				skipped.message = "Deferred to level pass (tag: \"" + p.tag + "\").";
				zoneSolution.participantResults[pi] = skipped;
				continue;
			}
			std::shared_ptr<IBitmapConverter> partConverter;
			GFXParams partGfx;
			PaletteParticipantResult r =
				FinalConvertParticipant(pi, p, projectFolder, targetSystem, targetMode, targetPaletteType, zoneFixed, overflowMethod, &partConverter, &partGfx);
			zoneSolution.participantResults[pi] = r;
			zoneBaseResults.push_back(r);
			if (partConverter)
				zoneBaseConverters[pi] = {partConverter, partGfx};
		}
		{
			PaletteTagSolution baseTs = SnapshotTagSolution("", zoneFixed, targetSystem, targetMode, zoneSolution.pensAvailable, zoneSolution.baseFit, (int)zoneUnion.size(),
															zoneOverflow, zoneBaseResults, std::move(zoneRemaps));
			for (auto& ce : zoneBaseConverters) {
				baseTs.converters[ce.first] = ce.second.conv;
				baseTs.converterGfx[ce.first] = ce.second.gfx;
			}
			//
			// Re-run Convert against the map-resident GFXParams so the palette's internal
			// raw pointers (stored by SetLockEnableArrays inside Convert) point into
			// stable map storage rather than the now-dead local partGfx stack variable.
			//
			for (auto& kv : baseTs.converters) {
				auto git = baseTs.converterGfx.find(kv.first);
				if (git != baseTs.converterGfx.end() && kv.second)
					kv.second->Convert(&git->second);
			}
			zoneSolution.tagSolutions.push_back(std::move(baseTs));
		}
		if (!zoneBaseResults.empty())
			zoneSolution.tagGroupResults.push_back({"", zoneBaseResults});
		//
		// Pass 3: per level-tag, collect Level participant colors, merge on top of zoneFixed, cap, final-convert
		//
		std::map<std::string, std::vector<int>> levelGroups;
		for (int pi = 0; pi < (int)zone.participants.size(); pi++) {
			const PaletteParticipant& p = zone.participants[pi];
			if (p.role == PaletteParticipantRole::Level)
				levelGroups[p.tag].push_back(pi);
		}
		for (auto& kv : levelGroups) {
			const std::string& tag = kv.first;
			const std::vector<int>& ids = kv.second;
			//
			// Build level union: start from zone fixed palette, add level participant colors
			//
			std::vector<int> levelUnion = zoneFixed;
			{
				std::vector<bool> seenLevel;
				int maxIdx = 0;
				for (int c : levelUnion)
					if (c > maxIdx)
						maxIdx = c;
				seenLevel.assign(maxIdx + 1, false);
				for (int c : levelUnion)
					seenLevel[c] = true;
				for (int pi : ids) {
					const PaletteParticipant& p = zone.participants[pi];
					PaletteParticipantResult dummy;
					std::vector<int> colors = CollectParticipantColors(pi, p, projectFolder, targetSystem, targetMode, targetPaletteType, nullptr, nullptr, dummy);
					for (int c : colors) {
						if (c >= (int)seenLevel.size())
							seenLevel.resize(c + 1, false);
						if (!seenLevel[c]) {
							seenLevel[c] = true;
							levelUnion.push_back(c);
						}
					}
				}
			}
			int levelOverflow = 0;
			std::vector<PaletteTagSolution::OverflowRemap> levelRemaps;
			std::vector<int> levelFixed = CapPalette(levelUnion, zoneSolution.pensAvailable, overflowMethod, zonePalette, levelOverflow, &levelRemaps);
			bool levelFit = (levelOverflow == 0);
			//
			// Remap transparent pen slots for level participants in this tag group
			//
			{
				std::vector<int> transSlotsLevel;
				std::vector<bool> neededLevel;
				for (int pi : ids) {
					const PaletteParticipant& p = zone.participants[pi];
					GFXParams pg = GetItemGfxParams(p, targetSystem, targetMode);
					int ts = pg.RParams.TransparentPen;
					if (ts < 0)
						continue;
					bool alreadyInList = false;
					for (int v : transSlotsLevel)
						if (v == ts) { alreadyInList = true; break; }
					if (!alreadyInList)
						transSlotsLevel.push_back(ts);
					PaletteParticipantResult dummy2;
					std::vector<int> pc = CollectParticipantColors(pi, p, projectFolder, targetSystem, targetMode, targetPaletteType, nullptr, nullptr, dummy2);
					for (int c : pc) {
						if (c >= (int)neededLevel.size())
							neededLevel.resize(c + 1, false);
						neededLevel[c] = true;
					}
				}
				RemapTransparentPenSlots(levelFixed, transSlotsLevel, neededLevel, lockedSlots);
			}
			//
			// Final-convert all Level participants for this tag against levelFixed
			//
			std::vector<PaletteParticipantResult> levelResults;
			std::unordered_map<int, ConverterEntry> levelConverters;
			for (int pi : ids) {
				const PaletteParticipant& p = zone.participants[pi];
				std::shared_ptr<IBitmapConverter> partConverter;
				GFXParams partGfx;
				PaletteParticipantResult r =
					FinalConvertParticipant(pi, p, projectFolder, targetSystem, targetMode, targetPaletteType, levelFixed, overflowMethod, &partConverter, &partGfx);
				levelResults.push_back(r);
				zoneSolution.participantResults[pi] = r;
				if (partConverter)
					levelConverters[pi] = {partConverter, partGfx};
			}
			PaletteTagSolution ts = SnapshotTagSolution(tag, levelFixed, targetSystem, targetMode, zoneSolution.pensAvailable, levelFit, (int)levelUnion.size(), levelOverflow,
														levelResults, std::move(levelRemaps));
			for (auto& ce : levelConverters) {
				ts.converters[ce.first] = ce.second.conv;
				ts.converterGfx[ce.first] = ce.second.gfx;
			}
			//
			// Re-run Convert against the map-resident GFXParams so the palette's internal
			// raw pointers point into stable map storage rather than the dead local stack.
			//
			for (auto& kv : ts.converters) {
				auto git = ts.converterGfx.find(kv.first);
				if (git != ts.converterGfx.end() && kv.second)
					kv.second->Convert(&git->second);
			}
			zoneSolution.tagSolutions.push_back(std::move(ts));
			zoneSolution.tagGroupResults.push_back({tag, levelResults});
		}
		return zoneSolution;
	}
	//
	// PaletteSolver::Solve -- public entry point
	//
	PaletteSolution PaletteSolver::Solve(const PaletteParams* params, const std::string& projectFolder) {
		PaletteSolution solution;
		if (!params || params->targetSystem.empty()) {
			solution.valid = false;
			solution.summary = "No target system configured.";
			return solution;
		}
		if (params->zones.empty()) {
			solution.valid = false;
			solution.summary = "No zones defined.";
			return solution;
		}
		//
		// Determine the pen budget by probing using the first zone's mode as representative.
		// Each zone is also probed individually in SolveZone for its own pen budget.
		// The probe converter is kept alive so its palette can serve CapPalette RGB lookups.
		//
		int pensAvailable = 0;
		std::shared_ptr<IBitmapConverter> globalProbeConverter;
		{
			const std::string& firstMode = params->zones[0].targetMode;
			GFXParams probeGfx = MakeGfxParams(params->targetSystem, firstMode, params->targetPaletteType);
			globalProbeConverter = Converters::GetBitmapConverter(&probeGfx);
			if (!globalProbeConverter) {
				solution.valid = false;
				solution.summary = "Unknown target system/mode: " + params->targetSystem + " / " + firstMode;
				return solution;
			}
			auto probeDummy = Image::ImageCreate(1, 1);
			if (probeDummy) {
				probeGfx.RParams.TargetWidth = 1;
				probeGfx.RParams.TargetHeight = 1;
				globalProbeConverter->SetOriginal(probeDummy);
				globalProbeConverter->Convert(&probeGfx);
			}
			pensAvailable = globalProbeConverter->GetPalette()->PaletteMaxColors();
		}
		IPaletteConverter* globalPalette = globalProbeConverter->GetPalette().get();
		solution.valid = true;
		int totalOk = 0;
		int totalOverflow = 0;
		int totalMissing = 0;
		//
		// Pass 1 (global Always): collect all Always participants from every zone,
		// build their union palette, cap it, and run the final conversion for each.
		// This produces a single fixed alwaysColors list shared by all zones and levels.
		//
		// Pre-seed the always union with pre-loaded locked colors so they occupy the
		// first pen slots in every solved palette, making them immutable across all zones.
		//
		std::vector<std::vector<int>> alwaysColorLists;
		if (!params->preloadedColors.empty()) {
			std::vector<int> preloadSeeds;
			for (int i = 0; i < (int)params->preloadedColors.size(); i++) {
				if (i < (int)params->preloadedLocked.size() && params->preloadedLocked[i] && params->preloadedColors[i] >= 0)
					preloadSeeds.push_back(params->preloadedColors[i]);
			}
			if (!preloadSeeds.empty())
				alwaysColorLists.push_back(std::move(preloadSeeds));
		}
		struct AlwaysEntry {
			int zi;
			int pi;
			std::vector<int> colors;
			PaletteParticipantResult result;
		};
		std::vector<AlwaysEntry> alwaysEntries;
		for (int zi = 0; zi < (int)params->zones.size(); zi++) {
			const PaletteZone& zone = params->zones[zi];
			for (int pi = 0; pi < (int)zone.participants.size(); pi++) {
				const PaletteParticipant& p = zone.participants[pi];
				if (p.role != PaletteParticipantRole::Always)
					continue;
				PaletteParticipantResult r;
				std::vector<int> colors = CollectParticipantColors(pi, p, projectFolder, params->targetSystem, zone.targetMode, params->targetPaletteType, nullptr, nullptr, r);
				alwaysColorLists.push_back(colors);
				alwaysEntries.push_back({zi, pi, colors, r});
				if (r.status == PaletteParticipantStatus::Missing || r.status == PaletteParticipantStatus::ImageLoadFailed)
					totalMissing++;
			}
		}
		//
		// Build union of all Always colors (in the order they were encountered)
		//
		std::vector<int> alwaysUnion = BuildUnionPalette(alwaysColorLists);
		int alwaysOverflow = 0;
		std::vector<PaletteTagSolution::OverflowRemap> alwaysRemaps;
		std::vector<int> alwaysFixed = CapPalette(alwaysUnion, pensAvailable, params->overflowMethod, globalPalette, alwaysOverflow, &alwaysRemaps);
		if (alwaysOverflow > 0)
			totalOverflow++;
		//
		// Build the locked-slot mask from preloaded pins so RemapTransparentPenSlots
		// never moves a color that the user has explicitly pinned to a specific pen slot.
		// lockedByPreload[i] is true when pen i was preloaded, locked and has a valid color.
		//
		std::vector<bool> lockedByPreload(pensAvailable, false);
		{
			int preloadSlot = 0;
			for (int i = 0; i < (int)params->preloadedColors.size(); i++) {
				if (i < (int)params->preloadedLocked.size() && params->preloadedLocked[i] && params->preloadedColors[i] >= 0) {
					if (preloadSlot < pensAvailable)
						lockedByPreload[preloadSlot] = true;
					preloadSlot++;
				}
			}
		}
		//
		// Remap transparent pen slots for Always participants.
		// Preloaded locked slots are protected from being moved.
		//
		{
			std::vector<int> transSlotsAlways;
			std::vector<bool> neededAlways;
			for (auto& ae : alwaysEntries) {
				const PaletteParticipant& p = params->zones[ae.zi].participants[ae.pi];
				GFXParams pg = GetItemGfxParams(p, params->targetSystem, params->zones[ae.zi].targetMode);
				int ts = pg.RParams.TransparentPen;
				if (ts < 0)
					continue;
				bool alreadyInList = false;
				for (int v : transSlotsAlways)
					if (v == ts) { alreadyInList = true; break; }
				if (!alreadyInList)
					transSlotsAlways.push_back(ts);
				for (int c : ae.colors) {
					if (c >= (int)neededAlways.size())
						neededAlways.resize(c + 1, false);
					neededAlways[c] = true;
				}
			}
			RemapTransparentPenSlots(alwaysFixed, transSlotsAlways, neededAlways, lockedByPreload);
		}
			//
			// Warn if participants across all zones use different transparent pen indices.
		// This is a design issue: ideally all transparency-using assets share one pen slot.
		//
		{
			std::vector<int> allTransPens;
			for (int zi = 0; zi < (int)params->zones.size(); zi++) {
				const PaletteZone& zone = params->zones[zi];
				for (int pi = 0; pi < (int)zone.participants.size(); pi++) {
					const PaletteParticipant& p = zone.participants[pi];
					GFXParams pg = GetItemGfxParams(p, params->targetSystem, zone.targetMode);
					int ts = pg.RParams.TransparentPen;
					if (ts < 0)
						continue;
					bool found = false;
					for (int v : allTransPens)
						if (v == ts) { found = true; break; }
					if (!found)
						allTransPens.push_back(ts);
				}
			}
			if ((int)allTransPens.size() > 1) {
				std::string penList;
				for (int i = 0; i < (int)allTransPens.size(); i++) {
					if (i > 0)
						penList += ", ";
					penList += std::to_string(allTransPens[i]);
				}
				solution.summary = "Warning: multiple transparent pen slots detected (" + penList + "). Ideally all transparency-using assets share the same pen slot. ";
			}
		}
		//
		// Final-convert all Always participants against the fixed Always palette.
		// alwaysConverters[zi][pi] = the resulting converter for UI display.
		//
		std::vector<std::unordered_map<int, std::pair<std::shared_ptr<IBitmapConverter>, GFXParams>>> alwaysConverters(params->zones.size());
		for (auto& ae : alwaysEntries) {
			if (ae.result.status == PaletteParticipantStatus::Missing || ae.result.status == PaletteParticipantStatus::ImageLoadFailed)
				continue;
			const PaletteParticipant& p = params->zones[ae.zi].participants[ae.pi];
			const std::string& zoneMode = params->zones[ae.zi].targetMode;
			std::shared_ptr<IBitmapConverter> partConverter;
			GFXParams partGfx;
			FinalConvertParticipant(ae.pi, p, projectFolder, params->targetSystem, zoneMode, params->targetPaletteType, alwaysFixed, params->overflowMethod, &partConverter,
									&partGfx);
			if (partConverter) {
				alwaysConverters[ae.zi][ae.pi] = {partConverter, partGfx};
				totalOk++;
			}
		}
		//
		// Passes 2 and 3 -- solve each zone using the fixed Always palette as base
		//
		for (int zi = 0; zi < (int)params->zones.size(); zi++) {
			PaletteZoneSolution zoneSolution =
				SolveZone(zi, params->zones[zi], params->targetSystem, params->targetPaletteType, projectFolder, params->overflowMethod, alwaysFixed, alwaysRemaps, lockedByPreload);
			//
			// Inject the Always-pass converters into the base tag solution (index 0)
			//
			if (!zoneSolution.tagSolutions.empty()) {
				PaletteTagSolution& base = zoneSolution.tagSolutions[0];
				for (auto& ce : alwaysConverters[zi]) {
					base.converters[ce.first] = ce.second.first;
					base.converterGfx[ce.first] = ce.second.second;
				}
				//
				// Re-run Convert against the map-resident GFXParams for Always entries so
				// the palette's raw pointers are valid in stable map storage.
				//
				for (auto& kv : alwaysConverters[zi]) {
					auto git = base.converterGfx.find(kv.first);
					if (git != base.converterGfx.end() && kv.second.first)
						kv.second.first->Convert(&git->second);
				}
			}
			//
			// Accumulate global counters from zone participant results
			//
			for (const auto& r : zoneSolution.participantResults) {
				if (r.status == PaletteParticipantStatus::Skipped)
					continue;
				if (r.status == PaletteParticipantStatus::OK)
					totalOk++;
				else if (r.status == PaletteParticipantStatus::PenOverflow)
					totalOverflow++;
				else if (r.status == PaletteParticipantStatus::Missing || r.status == PaletteParticipantStatus::ImageLoadFailed)
					totalMissing++;
			}
			solution.zones.push_back(std::move(zoneSolution));
		}
		//
		// Build summary string, preserving any warning already set (e.g. multi-transparent-pen)
		//
		std::string warnPrefix = solution.summary;
		if (totalOverflow > 0 || totalMissing > 0) {
			solution.valid = false;
			solution.summary = warnPrefix +
				"Solve completed with issues: " + std::to_string(totalOk) + " OK, " + std::to_string(totalOverflow) + " overflow(s), " + std::to_string(totalMissing) + " missing.";
		} else {
			solution.summary = warnPrefix + "All " + std::to_string(totalOk) + " participant(s) fit. Palette valid.";
		}
		return solution;
	}
	//
	// PaletteSolver::Validate -- write solved palette assignments back into each
	// participant's stored project GFXParams so subsequent conversions use the solved palette.
	//
	void PaletteSolver::Validate(const PaletteSolution& solution, const PaletteParams* params) {
		if (!params)
			return;
		bool modified = false;
		for (int zi = 0; zi < (int)solution.zones.size(); zi++) {
			if (zi >= (int)params->zones.size())
				break;
			const PaletteZone& zone = params->zones[zi];
			const PaletteZoneSolution& zoneSol = solution.zones[zi];
			for (const PaletteTagSolution& ts : zoneSol.tagSolutions) {
				for (const auto& kv : ts.converterGfx) {
					int pi = kv.first;
					if (pi < 0 || pi >= (int)zone.participants.size())
						continue;
					const PaletteParticipant& p = zone.participants[pi];
					if (p.buildItemName.empty())
						continue;
					//
					// Get a pointer to the item's stored GFXParams so we can write in-place
					//
					GFXParams* stored = nullptr;
					if (p.buildItemType == "Bitmap")
						Project::BitmapGetCfg(p.buildItemName, &stored);
					else if (p.buildItemType == "Tilemap")
						Project::TilesetGetCfg(p.buildItemName, &stored);
					else if (p.buildItemType == "Sprite")
						Project::SpriteGetCfg(p.buildItemName, &stored);
					if (!stored)
						continue;
					//
					// Copy solved palette lock/enable/color arrays into the stored params
					//
					const GFXParams& solved = kv.second;
					stored->SParams.PaletteLocked = solved.SParams.PaletteLocked;
					stored->SParams.PaletteEnabled = solved.SParams.PaletteEnabled;
					stored->SParams.PaletteColors = solved.SParams.PaletteColors;
					modified = true;
				}
			}
		}
		if (modified)
			Project::MarkAsModified();
	}
}
