// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Bitmap export engine -- runs AngelScript export scripts for bitmap items.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include "export.engine.h"
#include <convert/convert.palette.h>
#include <string>

namespace RetrodevLib {

	class Image;
	class IBitmapConverter;
	struct GFXParams;

	namespace ExportImpl {

		// -------------------------------------------------------------- //
		// BitmapExportContext -- wraps converter + params for script access //
		// -------------------------------------------------------------- //

		struct BitmapExportContext {
			IBitmapConverter* converter = nullptr;
			const GFXParams* params = nullptr;
			//
			// Serialised parameter values passed from the project's ExportParams::scriptParams.
			// Format: "key=value;key=value;..."
			// The script reads individual values via GetParam(key).
			//
			std::string scriptParams;
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
		// Register BitmapExportContext as a no-count ref type
		// Idempotent via g_engine.bitmapContextRegistered
		//
		void RegisterBitmapContextBinding(asIScriptEngine* engine);
		//
		// Load, compile, execute and discard a bitmap export script
		// The script must define: void Export(Image@, const string &in, BitmapExportContext@)
		// Returns false if any step fails; errors are logged via Log::Error
		//
		bool RunBitmapExport(const std::string& scriptPath, const std::string& outputPath, const std::string& scriptParams, Image* image, IBitmapConverter* converter,
							 const GFXParams* params);

	}
}
