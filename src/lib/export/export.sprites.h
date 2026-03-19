// --------------------------------------------------------------------------------------------------------------
//
//
//
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include "export.engine.h"
#include <convert/convert.palette.h>
#include <convert/convert.sprites.h>
#include <convert/convert.sprites.params.h>
#include <string>

namespace RetrodevLib {

	class Image;
	class IBitmapConverter;
	struct GFXParams;

	namespace ExportImpl {

		// ---------------------------------------------------------------- //
		// SpriteExportContext — wraps converter, extractor and params      //
		// for script access during sprite export                           //
		// ---------------------------------------------------------------- //

		struct SpriteExportContext {
			IBitmapConverter* converter = nullptr;
			const GFXParams* params = nullptr;
			ISpriteExtractor* spriteExtractor = nullptr;
			const SpriteExtractionParams* spriteParams = nullptr;
			//
			// Serialised parameter values passed from the project's ExportParams::scriptParams.
			// Format: "key=value;key=value;..."
			// The script reads individual values via GetParam(key).
			//
			std::string scriptParams;
			//
			// Number of sprites available in the extractor
			//
			int GetSpriteCount() const;
			//
			// Retrieve a specific sprite image by index (0-based).
			// Returns nullptr if index is out of range.
			//
			Image* GetSprite(int index) const;
			//
			// Width of the sprite at the given index in native pixels
			//
			int GetSpriteWidth(int index) const;
			//
			// Height of the sprite at the given index in native pixels
			//
			int GetSpriteHeight(int index) const;
			//
			// Name of the sprite at the given index
			//
			std::string GetSpriteName(int index) const;
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
		};

		//
		// Register SpriteExportContext as a no-count ref type.
		// Idempotent via g_engine.spriteContextRegistered.
		//
		void RegisterSpriteContextBinding(asIScriptEngine* engine);

		//
		// Load, compile, execute and discard a sprite export script.
		// The script must define: void Export(const string &in, SpriteExportContext@)
		//   outputPath — destination file path the script should write to
		//   context    — extractor and params for iterating sprites and querying details
		// Returns false if any step fails; errors are logged via Log::Error.
		//
		bool RunSpriteExport(const std::string& scriptPath, const std::string& outputPath, const std::string& scriptParams, IBitmapConverter* converter, const GFXParams* params,
							 ISpriteExtractor* spriteExtractor, const SpriteExtractionParams* spriteParams);

	} // namespace ExportImpl
} // namespace RetrodevLib
