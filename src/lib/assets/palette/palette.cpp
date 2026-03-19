// --------------------------------------------------------------------------------------------------------------
//
//
//
//
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
		// Do NOT seed any occupied slots — quantizer runs freely to find this image's best colors.
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
		if (gfx.RParams.TargetWidth == 0 || gfx.RParams.TargetHeight == 0) {
			gfx.RParams.TargetWidth = 1;
			gfx.RParams.TargetHeight = 1;
		}
		converter->SetOriginal(image);
		converter->Convert(&gfx);
		//
		// Collect unique assigned color indices in pen order
		//
		auto palette = converter->GetPalette();
		std::vector<int> colors;
		for (int i = 0; i < maxPens; i++) {
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
															const std::vector<int>& fixedPalette, std::shared_ptr<IBitmapConverter>* outConverter, GFXParams* outGfx) {
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
		result.status = PaletteParticipantStatus::OK;
		result.message = "OK (fitted to capped palette).";
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
	// Apply the cap method to reduce unionPalette to at most pensAvailable entries.
	// Returns the capped palette and sets overflow to the number of dropped colors.
	//
	static std::vector<int> CapPalette(const std::vector<int>& unionPalette, int pensAvailable, PaletteCapMethod method, int& overflow) {
		if ((int)unionPalette.size() <= pensAvailable) {
			overflow = 0;
			return unionPalette;
		}
		overflow = (int)unionPalette.size() - pensAvailable;
		if (method == PaletteCapMethod::HardCap) {
			//
			// Truncate: keep the first pensAvailable entries (highest priority colors survive)
			//
			return std::vector<int>(unionPalette.begin(), unionPalette.begin() + pensAvailable);
		}
		//
		// Default fallback to HardCap for any future unimplemented methods
		//
		return std::vector<int>(unionPalette.begin(), unionPalette.begin() + pensAvailable);
	}
	//
	// Helper: take a snapshot into a PaletteTagSolution for UI display.
	//
	static PaletteTagSolution SnapshotTagSolution(const std::string& tag, const std::vector<int>& fixedPalette, const std::string& targetSystem, const std::string& targetMode,
												  int pensAvailable, bool fit, int unionSize, int overflow, const std::vector<PaletteParticipantResult>& results) {
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
		//
		// Convert the fixed palette back into occupiedSlots (pen i → fixedPalette[i], rest -1)
		//
		ts.occupiedSlots.assign(pensAvailable, -1);
		for (int i = 0; i < (int)fixedPalette.size() && i < pensAvailable; i++)
			ts.occupiedSlots[i] = fixedPalette[i];
		return ts;
	}
	//
	// Solve one zone given a list of Always color lists already collected globally.
	// alwaysColors: union-capped palette from the global Always pass (shared across all zones/levels).
	// Pass 2: collect Zone/ScreenZone participant colors, merge with alwaysColors, cap, final-convert.
	// Pass 3: per level-tag, collect Level participant colors, merge on top of zone base, cap, final-convert.
	//
	static PaletteZoneSolution SolveZone(int zoneIndex, const PaletteZone& zone, const std::string& targetSystem, const std::string& targetPaletteType,
										 const std::string& projectFolder, PaletteCapMethod capMethod, const std::vector<int>& alwaysColors) {
		const std::string& targetMode = zone.targetMode;
		PaletteZoneSolution zoneSolution;
		zoneSolution.zoneIndex = zoneIndex;
		zoneSolution.participantResults.resize(zone.participants.size());
		//
		// Probe PaletteMaxColors by creating a one-shot primed converter
		//
		{
			GFXParams probeGfx = MakeGfxParams(targetSystem, targetMode, targetPaletteType);
			auto probeConverter = Converters::GetBitmapConverter(&probeGfx);
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
		//
		// Helper struct for capturing converters produced during the zone solve passes
		//
		struct ConverterEntry {
			std::shared_ptr<IBitmapConverter> conv;
			GFXParams gfx;
		};
		//
		// Collect the color lists for every ScreenZone participant (Pass 2: zone base)
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
				if (p.role != PaletteParticipantRole::ScreenZone)
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
		std::vector<int> zoneFixed = CapPalette(zoneUnion, zoneSolution.pensAvailable, capMethod, zoneOverflow);
		zoneSolution.baseFit = (zoneOverflow == 0);
		//
		// Final-convert all Always + ScreenZone participants against the zone fixed palette
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
			PaletteParticipantResult r = FinalConvertParticipant(pi, p, projectFolder, targetSystem, targetMode, targetPaletteType, zoneFixed, &partConverter, &partGfx);
			zoneSolution.participantResults[pi] = r;
			zoneBaseResults.push_back(r);
			if (partConverter)
				zoneBaseConverters[pi] = {partConverter, partGfx};
		}
		{
			PaletteTagSolution baseTs = SnapshotTagSolution("", zoneFixed, targetSystem, targetMode, zoneSolution.pensAvailable, zoneSolution.baseFit, (int)zoneUnion.size(),
															zoneOverflow, zoneBaseResults);
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
			std::vector<int> levelFixed = CapPalette(levelUnion, zoneSolution.pensAvailable, capMethod, levelOverflow);
			bool levelFit = (levelOverflow == 0);
			//
			// Final-convert all Level participants for this tag against levelFixed
			//
			std::vector<PaletteParticipantResult> levelResults;
			std::unordered_map<int, ConverterEntry> levelConverters;
			for (int pi : ids) {
				const PaletteParticipant& p = zone.participants[pi];
				std::shared_ptr<IBitmapConverter> partConverter;
				GFXParams partGfx;
				PaletteParticipantResult r = FinalConvertParticipant(pi, p, projectFolder, targetSystem, targetMode, targetPaletteType, levelFixed, &partConverter, &partGfx);
				levelResults.push_back(r);
				zoneSolution.participantResults[pi] = r;
				if (partConverter)
					levelConverters[pi] = {partConverter, partGfx};
			}
			PaletteTagSolution ts =
				SnapshotTagSolution(tag, levelFixed, targetSystem, targetMode, zoneSolution.pensAvailable, levelFit, (int)levelUnion.size(), levelOverflow, levelResults);
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
	// PaletteSolver::Solve — public entry point
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
		//
		int pensAvailable = 0;
		{
			const std::string& firstMode = params->zones[0].targetMode;
			GFXParams probeGfx = MakeGfxParams(params->targetSystem, firstMode, params->targetPaletteType);
			auto probeConverter = Converters::GetBitmapConverter(&probeGfx);
			if (!probeConverter) {
				solution.valid = false;
				solution.summary = "Unknown target system/mode: " + params->targetSystem + " / " + firstMode;
				return solution;
			}
			auto probeDummy = Image::ImageCreate(1, 1);
			if (probeDummy) {
				probeGfx.RParams.TargetWidth = 1;
				probeGfx.RParams.TargetHeight = 1;
				probeConverter->SetOriginal(probeDummy);
				probeConverter->Convert(&probeGfx);
			}
			pensAvailable = probeConverter->GetPalette()->PaletteMaxColors();
		}
		solution.valid = true;
		int totalOk = 0;
		int totalOverflow = 0;
		int totalMissing = 0;
		//
		// Pass 1 (global Always): collect all Always participants from every zone,
		// build their union palette, cap it, and run the final conversion for each.
		// This produces a single fixed alwaysColors list shared by all zones and levels.
		//
		std::vector<std::vector<int>> alwaysColorLists;
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
		std::vector<int> alwaysFixed = CapPalette(alwaysUnion, pensAvailable, params->capMethod, alwaysOverflow);
		if (alwaysOverflow > 0)
			totalOverflow++;
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
			FinalConvertParticipant(ae.pi, p, projectFolder, params->targetSystem, zoneMode, params->targetPaletteType, alwaysFixed, &partConverter, &partGfx);
			if (partConverter) {
				alwaysConverters[ae.zi][ae.pi] = {partConverter, partGfx};
				totalOk++;
			}
		}
		//
		// Passes 2 and 3 — solve each zone using the fixed Always palette as base
		//
		for (int zi = 0; zi < (int)params->zones.size(); zi++) {
			PaletteZoneSolution zoneSolution = SolveZone(zi, params->zones[zi], params->targetSystem, params->targetPaletteType, projectFolder, params->capMethod, alwaysFixed);
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
		// Build summary string
		//
		if (totalOverflow > 0 || totalMissing > 0) {
			solution.valid = false;
			solution.summary =
				"Solve completed with issues: " + std::to_string(totalOk) + " OK, " + std::to_string(totalOverflow) + " overflow(s), " + std::to_string(totalMissing) + " missing.";
		} else {
			solution.summary = "All " + std::to_string(totalOk) + " participant(s) fit. Palette valid.";
		}
		return solution;
	}
	//
	// PaletteSolver::Validate — write solved palette assignments back into each
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
} // namespace RetrodevLib
