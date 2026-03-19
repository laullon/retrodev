// --------------------------------------------------------------------------------------------------------------
//
//
//
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <angelscript.h>
#include <scriptbuilder.h>
#include <string>
#include <vector>
#include <log/log.h>
#include "export.h"

namespace RetrodevLib {
	namespace ExportImpl {

		//
		// Shared error collection — written to by MessageCallback and all helpers
		//
		extern std::vector<std::string> g_errors;
		extern bool g_hasError;
		//
		// Append msg to g_errors, set g_hasError and emit via Log::Error
		//
		void ReportError(const std::string& msg);
		//
		// Ensure the parent directory of outputPath exists, creating it (and any
		// missing ancestors) if necessary. Returns false and reports an error on failure.
		//
		bool EnsureOutputDirectory(const std::string& outputPath);
		//
		// AngelScript message callback — routes compiler and runtime messages
		// to g_errors and the Log facility
		//
		void MessageCallback(const asSMessageInfo* msg, void* param);

		// -------------------------------------------------------------- //
		// ScriptEngine — AngelScript engine lifecycle and module building //
		// -------------------------------------------------------------- //

		class ScriptEngine {
		public:
			asIScriptEngine* engine = nullptr;
			CScriptBuilder builder;
			bool initialized = false;
			//
			// Per-binding registration flags — each resets to false on Shutdown
			//
			bool rgbColorRegistered = false;
			bool imageRegistered = false;
			bool bitmapContextRegistered = false;
			bool tilesetContextRegistered = false;
			bool spriteContextRegistered = false;
			bool mapContextRegistered = false;
			bool paletteRegistered = false;
			bool addonsRegistered = false;
			//
			// Initialize the engine, register std::string and route messages to g_errors
			//
			bool Initialize();
			//
			// Shutdown and release all engine resources; resets all flags
			//
			void Shutdown();
			//
			// Start a new named module, discarding any previous one with the same name
			//
			bool BeginModule(const std::string& name);
			//
			// Add a script section from a file on disk
			//
			bool AddSectionFromFile(const std::string& path);
			//
			// Compile all sections added since BeginModule
			//
			bool Build();
			//
			// Discard a compiled module; safe to call if the module does not exist
			//
			void Discard(const std::string& name);
			//
			// Read the optional description from a script file without compiling it.
			// Scans for a line of the form:  // @description <text>
			// Returns the trimmed text after the tag, or an empty string if absent.
			//
			static std::string GetScriptDescription(const std::string& scriptPath);
			//
			// Scan a script file for all recognised // @tag metadata lines in a single pass.
			// Never touches the AngelScript engine — safe to call before Initialize().
			//
			static ScriptMetadata GetScriptMetadata(const std::string& scriptPath);
		};

		extern ScriptEngine g_engine;

		//
		// Register RgbColor as a 4-byte POD value type (r, g, b, a uint8 properties)
		// Idempotent via g_engine.rgbColorRegistered
		//
		void RegisterRgbColorBinding(asIScriptEngine* engine);
		//
		// Register Image as a no-count ref type (GetWidth, GetHeight, GetPixelColor, Lock, Unlock)
		// Idempotent via g_engine.imageRegistered
		//
		void RegisterImageBinding(asIScriptEngine* engine);
		//
		// Register IPaletteConverter as a no-count ref type (PaletteMaxColors, PenGetColor,
		// GetSystemIndexByColor). Idempotent via g_engine.paletteRegistered.
		//
		void RegisterPaletteBinding(asIScriptEngine* engine);
		//
		// Register scriptfile, scriptarray and scriptmath addons.
		// Idempotent via g_engine.addonsRegistered.
		//
		void RegisterAddons(asIScriptEngine* engine);

	} // namespace ExportImpl
} // namespace RetrodevLib
