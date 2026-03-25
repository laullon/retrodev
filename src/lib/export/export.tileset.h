// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Tileset export engine -- runs AngelScript export scripts for tileset items.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include "export.engine.h"
#include <convert/convert.palette.h>
#include <convert/convert.tileset.h>
#include <convert/convert.tileset.params.h>
#include <string>

namespace RetrodevLib {

	class Image;
	class IBitmapConverter;
	struct GFXParams;

	namespace ExportImpl {

		// ---------------------------------------------------------------- //
		// TilesetExportContext -- wraps converter, extractor and params      //
		// for script access during tileset export                           //
		// ---------------------------------------------------------------- //

		struct TilesetExportContext {
			IBitmapConverter* converter = nullptr;
			const GFXParams* params = nullptr;
			ITileExtractor* tileExtractor = nullptr;
			const TileExtractionParams* tileParams = nullptr;
			//
			// Serialised parameter values passed from the project's ExportParams::scriptParams.
			// Format: "key=value;key=value;..."
			// The script reads individual values via GetParam(key).
			//
			std::string scriptParams;
			//
			// Number of tiles available in the extractor
			//
			int GetTileCount() const;
			//
			// Retrieve a specific tile image by index (0-based).
			// Returns nullptr if index is out of range.
			//
			Image* GetTile(int index) const;
			//
			// Width of a single tile in native pixels
			//
			int GetTileWidth() const;
			//
			// Height of a single tile in native pixels
			//
			int GetTileHeight() const;
			//
			// Width of the converted image in native target units
			//
			int GetNativeWidth() const;
			//
			// Height of the converted image in native target units
			//
			int GetNativeHeight() const;
			//
			// Target system video mode name (e.g. "Mode 0")
			//
			std::string GetTargetMode() const;
			//
			// Target system name (e.g. "Amstrad CPC")
			//
			std::string GetTargetSystem() const;
			//
			// Retrieve the palette converter so scripts can look up pen indices
			//
			IPaletteConverter* GetPalette() const;
			//
			// Retrieve the value for a named parameter from the serialised scriptParams string.
			// Returns the value string if the key is found, or an empty string if absent.
			//
			std::string GetParam(const std::string& key) const;
			//
			// Transparency settings -- forwarded from ResizeParams so export scripts can
			// handle transparent pixels without falling back to a per-pixel warning.
			//
			bool GetUseTransparentColor() const;
			int GetTransparentPen() const;
		};

		//
		// Register TilesetExportContext as a no-count ref type.
		// Idempotent via g_engine.tilesetContextRegistered.
		//
		void RegisterTilesetContextBinding(asIScriptEngine* engine);

		//
		// Load, compile, execute and discard a tileset export script.
		// The script must define: void Export(const string &in, TilesetExportContext@)
		//   outputPath -- destination file path the script should write to
		//   context    -- extractor and params for iterating tiles and querying details
		// Returns false if any step fails; errors are logged via Log::Error.
		//
		bool RunTilesetExport(const std::string& scriptPath, const std::string& outputPath, const std::string& scriptParams, IBitmapConverter* converter, const GFXParams* params,
							  ITileExtractor* tileExtractor, const TileExtractionParams* tileParams);

	}
}
