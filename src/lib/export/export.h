// --------------------------------------------------------------------------------------------------------------
//
//
//
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <string>
#include <vector>
#include <log/log.h>

namespace RetrodevLib {

	class Image;
	class IBitmapConverter;
	class ITileExtractor;
	class ISpriteExtractor;
	struct GFXParams;
	struct TileExtractionParams;
	struct SpriteExtractionParams;
	struct MapParams;

	//
	// A single user-configurable parameter declared by a script via // @param.
	// The UI builds controls from these definitions; values are stored separately
	// as a serialised key=value string and passed back to the script at runtime.
	//
	struct ScriptParamDef {
		//
		// Internal key used in the serialised params string and by GetParam() in the script
		//
		std::string key;
		//
		// Type token: "bool", "int", "string", or "combo"
		//
		std::string type;
		//
		// Default value as a string ("true"/"false" for bool, digits for int,
		// text for string, first option for combo)
		//
		std::string defaultValue;
		//
		// Human-readable label shown next to the control in the UI
		//
		std::string label;
		//
		// Non-empty only for combo type: the ordered list of selectable values
		// parsed from the pipe-separated token in the @param line.
		//
		std::vector<std::string> options;
	};

	//
	// Metadata read from a script file via // @tag lines.
	// All fields are optional; absent tags produce empty strings / empty vector.
	//
	struct ScriptMetadata {
		//
		// // @description <text>  — human-readable one-line description
		//
		std::string description;
		//
		// // @exporter <type>  — kind of data exported (bitmap, map, sprites, tiles)
		//
		std::string exporter;
		//
		// // @target <system>  — target hardware identifier (e.g. amstrad.cpc, spectrum)
		//
		std::string target;
		//
		// // @param <key> <type> <default> <label...>  — user-configurable parameters
		//
		std::vector<ScriptParamDef> params;
	};

	//
	//
	//
	class ExportEngine {
	public:
		//
		// Initialize the script engine and register built-in types
		// Idempotent: safe to call multiple times
		// Returns false if the engine could not be created
		//
		static bool Initialize();
		//
		// Shutdown the script engine and release all resources
		// Resets all internal state so a subsequent Initialize() starts clean
		//
		static void Shutdown();
		//
		// Returns true if any compile or runtime error occurred since the last ClearErrors()
		//
		static bool HasErrors();
		//
		// Returns all collected messages (errors, warnings, info) in order
		// Format: "[Type] section (line, col): message"
		//
		static const std::vector<std::string>& GetErrors();
		//
		// Clear all collected messages and reset the error flag
		//
		static void ClearErrors();
		//
		// Scan a script file for all recognised // @tag metadata lines in a single pass.
		// Never touches the AngelScript engine — safe to call before Initialize().
		//
		static ScriptMetadata GetScriptMetadata(const std::string& scriptPath);
		//
		// Convenience wrapper — returns only the @description field.
		//
		static std::string GetScriptDescription(const std::string& scriptPath);
		//
		// Execute a bitmap export script
		// The script must define: void Export(Image@, const string &in, BitmapExportContext@)
		//   image      — the converted image, ready for pixel access
		//   outputPath — destination file path the script should write to
		//   context    — converter and params for querying mode and resolution details
		// Returns false if loading, compilation or execution fails
		//
		static bool ExportBitmap(const std::string& scriptPath, const std::string& outputPath, const std::string& scriptParams, Image* image, IBitmapConverter* converter,
								 const GFXParams* params);
		//
		// Execute a tileset export script
		// The script must define: void Export(const string &in, TilesetExportContext@)
		//   outputPath     — destination file path the script should write to
		//   context        — extractor and params for iterating tiles and querying details
		// Returns false if loading, compilation or execution fails
		//
		static bool ExportTileset(const std::string& scriptPath, const std::string& outputPath, const std::string& scriptParams, IBitmapConverter* converter,
								  const GFXParams* params, ITileExtractor* tileExtractor, const TileExtractionParams* tileParams);
		//
		// Execute a sprite export script
		// The script must define: void Export(const string &in, SpriteExportContext@)
		//   outputPath  — destination file path the script should write to
		//   context     — extractor and params for iterating sprites and querying details
		// Returns false if loading, compilation or execution fails
		//
		static bool ExportSprites(const std::string& scriptPath, const std::string& outputPath, const std::string& scriptParams, IBitmapConverter* converter,
								  const GFXParams* params, ISpriteExtractor* spriteExtractor, const SpriteExtractionParams* spriteParams);
		//
		// Execute a map export script
		// The script must define: void Export(const string &in, MapExportContext@)
		//   outputPath — destination file path the script should write to
		//   context    — map params for iterating layers and cells
		// Returns false if loading, compilation or execution fails
		//
		static bool ExportMap(const std::string& scriptPath, const std::string& outputPath, const std::string& scriptParams, const MapParams* mapParams);
	};

} // namespace RetrodevLib