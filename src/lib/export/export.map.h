// --------------------------------------------------------------------------------------------------------------
//
//
//
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include "export.engine.h"
#include <string>
#include <vector>

namespace RetrodevLib {

	struct MapParams;

	namespace ExportImpl {

		// ---------------------------------------------------------------- //
		// MapExportContext — wraps MapParams for script access              //
		// during map export                                                 //
		// ---------------------------------------------------------------- //

		struct MapExportContext {
			//
			// Pointer to the map parameters (layers, tilesets, viewport)
			//
			const MapParams* mapParams = nullptr;
			//
			// Serialised parameter values passed from the project's ExportParams::scriptParams.
			// Format: "key=value;key=value;..."
			// The script reads individual values via GetParam(key).
			//
			std::string scriptParams;
			//
			// Number of layers in the map
			//
			int GetLayerCount() const;
			//
			// Name of a layer by index
			//
			std::string GetLayerName(int layerIndex) const;
			//
			// Width in tiles of a layer by index
			//
			int GetLayerWidth(int layerIndex) const;
			//
			// Height in tiles of a layer by index
			//
			int GetLayerHeight(int layerIndex) const;
			//
			// Retrieve the raw cell word at (col, row) in a layer.
			// Returns 0 for empty cells or out-of-range coordinates.
			// Encoding: top 4 bits = tilesetSlotIndex + 1 (0 = empty), bottom 12 bits = tileIndex.
			//
			int GetCell(int layerIndex, int col, int row) const;
			//
			// Decode the tileset slot index (0-based) from a cell word.
			// Returns -1 for empty cells.
			//
			int GetCellTilesetIndex(int cellVal) const;
			//
			// Decode the tile index (0-based) from a cell word.
			//
			int GetCellTileIndex(int cellVal) const;
			//
			// Viewable area width in tiles (shared across all layers)
			//
			int GetViewWidth() const;
			//
			// Viewable area height in tiles (shared across all layers)
			//
			int GetViewHeight() const;
			//
			// Retrieve the value for a named parameter from the serialised scriptParams string.
			// Returns the value string if the key is found, or an empty string if absent.
			//
			std::string GetParam(const std::string& key) const;
		};

		//
		// Register MapExportContext as a no-count ref type.
		// Idempotent via g_engine.mapContextRegistered.
		//
		void RegisterMapContextBinding(asIScriptEngine* engine);

		//
		// Load, compile, execute and discard a map export script.
		// The script must define: void Export(const string &in, MapExportContext@)
		//   outputPath — destination file path the script should write to
		//   context    — map params for iterating layers and cells
		// Returns false if any step fails; errors are logged via Log::Error.
		//
		bool RunMapExport(const std::string& scriptPath, const std::string& outputPath, const std::string& scriptParams, const MapParams* mapParams);

	} // namespace ExportImpl
} // namespace RetrodevLib
