// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Tileset export engine -- runs AngelScript export scripts for tileset items.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#include "export.tileset.h"
#include <assets/image/image.h>
#include <assets/image/image.color.h>
#include <convert/convert.bitmap.h>
#include <convert/convert.bitmap.params.h>
#include <log/log.h>

namespace RetrodevLib {
	namespace ExportImpl {

		// ---------------------------------------------------------------- //
		// TilesetExportContext                                              //
		// ---------------------------------------------------------------- //

		std::string TilesetExportContext::GetParam(const std::string& key) const {
			//
			// Walk the "key=value;key=value;..." string looking for an exact key match
			//
			const std::string& s = scriptParams;
			std::size_t pos = 0;
			while (pos < s.size()) {
				std::size_t semi = s.find(';', pos);
				if (semi == std::string::npos)
					semi = s.size();
				std::size_t eq = s.find('=', pos);
				if (eq != std::string::npos && eq < semi) {
					if (s.substr(pos, eq - pos) == key)
						return s.substr(eq + 1, semi - eq - 1);
				}
				pos = semi + 1;
			}
			return std::string();
		}
		int TilesetExportContext::GetTileCount() const {
			return tileExtractor ? tileExtractor->GetTileCount() : 0;
		}
		Image* TilesetExportContext::GetTile(int index) const {
			if (!tileExtractor)
				return nullptr;
			auto tile = tileExtractor->GetTile(index);
			return tile ? tile.get() : nullptr;
		}
		int TilesetExportContext::GetTileWidth() const {
			return tileParams ? tileParams->TileWidth : 0;
		}
		int TilesetExportContext::GetTileHeight() const {
			return tileParams ? tileParams->TileHeight : 0;
		}
		int TilesetExportContext::GetNativeWidth() const {
			return (converter && params) ? converter->GetNativeWidth(params) : 0;
		}
		int TilesetExportContext::GetNativeHeight() const {
			return (converter && params) ? converter->GetNativeHeight(params) : 0;
		}
		std::string TilesetExportContext::GetTargetMode() const {
			return params ? params->SParams.TargetMode : std::string();
		}
		std::string TilesetExportContext::GetTargetSystem() const {
			return params ? params->SParams.TargetSystem : std::string();
		}
		IPaletteConverter* TilesetExportContext::GetPalette() const {
			return converter ? converter->GetPalette().get() : nullptr;
		}
		bool TilesetExportContext::GetUseTransparentColor() const {
			return params ? params->RParams.UseTransparentColor : false;
		}
		int TilesetExportContext::GetTransparentPen() const {
			return params ? params->RParams.TransparentPen : -1;
		}

		// ---------------------------------------------------------------- //
		// RegisterTilesetContextBinding                                     //
		// ---------------------------------------------------------------- //

		//
		// Generic wrappers -- required by AS_MAX_PORTABILITY builds
		//
		static void TilesetExportContext_GetTileCount_Generic(asIScriptGeneric* gen) {
			gen->SetReturnDWord((asDWORD) static_cast<TilesetExportContext*>(gen->GetObject())->GetTileCount());
		}
		static void TilesetExportContext_GetTile_Generic(asIScriptGeneric* gen) {
			int index = gen->GetArgDWord(0);
			gen->SetReturnAddress(static_cast<TilesetExportContext*>(gen->GetObject())->GetTile(index));
		}
		static void TilesetExportContext_GetTileWidth_Generic(asIScriptGeneric* gen) {
			gen->SetReturnDWord((asDWORD) static_cast<TilesetExportContext*>(gen->GetObject())->GetTileWidth());
		}
		static void TilesetExportContext_GetTileHeight_Generic(asIScriptGeneric* gen) {
			gen->SetReturnDWord((asDWORD) static_cast<TilesetExportContext*>(gen->GetObject())->GetTileHeight());
		}
		static void TilesetExportContext_GetNativeWidth_Generic(asIScriptGeneric* gen) {
			gen->SetReturnDWord((asDWORD) static_cast<TilesetExportContext*>(gen->GetObject())->GetNativeWidth());
		}
		static void TilesetExportContext_GetNativeHeight_Generic(asIScriptGeneric* gen) {
			gen->SetReturnDWord((asDWORD) static_cast<TilesetExportContext*>(gen->GetObject())->GetNativeHeight());
		}
		static void TilesetExportContext_GetTargetMode_Generic(asIScriptGeneric* gen) {
			new (gen->GetAddressOfReturnLocation()) std::string(static_cast<TilesetExportContext*>(gen->GetObject())->GetTargetMode());
		}
		static void TilesetExportContext_GetTargetSystem_Generic(asIScriptGeneric* gen) {
			new (gen->GetAddressOfReturnLocation()) std::string(static_cast<TilesetExportContext*>(gen->GetObject())->GetTargetSystem());
		}
		static void TilesetExportContext_GetParam_Generic(asIScriptGeneric* gen) {
			const std::string& key = *static_cast<std::string*>(gen->GetArgObject(0));
			new (gen->GetAddressOfReturnLocation()) std::string(static_cast<TilesetExportContext*>(gen->GetObject())->GetParam(key));
		}
		static void TilesetExportContext_GetPalette_Generic(asIScriptGeneric* gen) {
			gen->SetReturnAddress(static_cast<TilesetExportContext*>(gen->GetObject())->GetPalette());
		}
		static void TilesetExportContext_GetUseTransparentColor_Generic(asIScriptGeneric* gen) {
			gen->SetReturnByte(static_cast<TilesetExportContext*>(gen->GetObject())->GetUseTransparentColor() ? 1 : 0);
		}
		static void TilesetExportContext_GetTransparentPen_Generic(asIScriptGeneric* gen) {
			gen->SetReturnDWord((asDWORD) static_cast<TilesetExportContext*>(gen->GetObject())->GetTransparentPen());
		}
		//
		void RegisterTilesetContextBinding(asIScriptEngine* engine) {
			if (g_engine.tilesetContextRegistered)
				return;
			//
			// TilesetExportContext -- ref type, no script-side reference counting
			//
			engine->RegisterObjectType("TilesetExportContext", 0, asOBJ_REF | asOBJ_NOCOUNT);
			engine->RegisterObjectMethod("TilesetExportContext", "int GetTileCount() const", asFUNCTION(TilesetExportContext_GetTileCount_Generic), asCALL_GENERIC);
			engine->RegisterObjectMethod("TilesetExportContext", "Image@ GetTile(int index) const", asFUNCTION(TilesetExportContext_GetTile_Generic), asCALL_GENERIC);
			engine->RegisterObjectMethod("TilesetExportContext", "int GetTileWidth() const", asFUNCTION(TilesetExportContext_GetTileWidth_Generic), asCALL_GENERIC);
			engine->RegisterObjectMethod("TilesetExportContext", "int GetTileHeight() const", asFUNCTION(TilesetExportContext_GetTileHeight_Generic), asCALL_GENERIC);
			engine->RegisterObjectMethod("TilesetExportContext", "int GetNativeWidth() const", asFUNCTION(TilesetExportContext_GetNativeWidth_Generic), asCALL_GENERIC);
			engine->RegisterObjectMethod("TilesetExportContext", "int GetNativeHeight() const", asFUNCTION(TilesetExportContext_GetNativeHeight_Generic), asCALL_GENERIC);
			engine->RegisterObjectMethod("TilesetExportContext", "string GetTargetMode() const", asFUNCTION(TilesetExportContext_GetTargetMode_Generic), asCALL_GENERIC);
			engine->RegisterObjectMethod("TilesetExportContext", "string GetTargetSystem() const", asFUNCTION(TilesetExportContext_GetTargetSystem_Generic), asCALL_GENERIC);
			engine->RegisterObjectMethod("TilesetExportContext", "string GetParam(const string &in) const", asFUNCTION(TilesetExportContext_GetParam_Generic), asCALL_GENERIC);
			engine->RegisterObjectMethod("TilesetExportContext", "Palette@ GetPalette() const", asFUNCTION(TilesetExportContext_GetPalette_Generic), asCALL_GENERIC);
			engine->RegisterObjectMethod("TilesetExportContext", "bool GetUseTransparentColor() const", asFUNCTION(TilesetExportContext_GetUseTransparentColor_Generic), asCALL_GENERIC);
			engine->RegisterObjectMethod("TilesetExportContext", "int GetTransparentPen() const", asFUNCTION(TilesetExportContext_GetTransparentPen_Generic), asCALL_GENERIC);
			g_engine.tilesetContextRegistered = true;
		}

		// ---------------------------------------------------------------- //
		// RunTilesetExport                                                  //
		// ---------------------------------------------------------------- //

		bool RunTilesetExport(const std::string& scriptPath, const std::string& outputPath, const std::string& scriptParams, IBitmapConverter* converter, const GFXParams* params,
							  ITileExtractor* tileExtractor, const TileExtractionParams* tileParams) {
			if (!g_engine.initialized) {
				ReportError("[Script] ExportTileset: engine not initialized -- call ExportEngine::Initialize() first");
				return false;
			}
			if (!EnsureOutputDirectory(outputPath))
				return false;
			//
			// Register all bindings required by tileset export scripts
			//
			RegisterRgbColorBinding(g_engine.engine);
			RegisterImageBinding(g_engine.engine);
			RegisterPaletteBinding(g_engine.engine);
			RegisterTilesetContextBinding(g_engine.engine);
			//
			// Load, compile and build the script into a transient module
			//
			const std::string moduleName = "ExportTileset";
			if (!g_engine.BeginModule(moduleName))
				return false;
			if (!g_engine.AddSectionFromFile(scriptPath))
				return false;
			if (!g_engine.Build())
				return false;
			//
			// Resolve the script entry point
			//
			asIScriptModule* mod = g_engine.engine->GetModule(moduleName.c_str());
			if (mod == nullptr) {
				ReportError("[Script] ExportTileset: module not found after build");
				g_engine.Discard(moduleName);
				return false;
			}
			const std::string funcDecl = "void Export(const string &in, TilesetExportContext@)";
			asIScriptFunction* func = mod->GetFunctionByDecl(funcDecl.c_str());
			if (func == nullptr) {
				ReportError("[Script] ExportTileset: function '" + funcDecl + "' not found in script '" + scriptPath + "'");
				g_engine.Discard(moduleName);
				return false;
			}
			//
			// Build the context object and invoke the script function
			//
			TilesetExportContext exportCtx;
			exportCtx.converter = converter;
			exportCtx.params = params;
			exportCtx.tileExtractor = tileExtractor;
			exportCtx.tileParams = tileParams;
			exportCtx.scriptParams = scriptParams;
			asIScriptContext* ctx = g_engine.engine->CreateContext();
			if (ctx == nullptr) {
				ReportError("[Script] ExportTileset: failed to create script context");
				g_engine.Discard(moduleName);
				return false;
			}
			ctx->Prepare(func);
			ctx->SetArgObject(0, const_cast<std::string*>(&outputPath));
			ctx->SetArgObject(1, &exportCtx);
			int r = ctx->Execute();
			ctx->Release();
			//
			// Always discard so the next call reloads the script from disk
			//
			g_engine.Discard(moduleName);
			if (r != asEXECUTION_FINISHED) {
				ReportError("[Script] ExportTileset: execution did not finish normally (r=" + std::to_string(r) + ")");
				return false;
			}
			return true;
		}

	}
}
