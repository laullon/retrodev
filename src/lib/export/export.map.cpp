// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Map export engine -- runs AngelScript export scripts for map items.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#include "export.map.h"
#include <assets/map/map.params.h>
#include <log/log.h>

namespace RetrodevLib {
	namespace ExportImpl {

		// ---------------------------------------------------------------- //
		// MapExportContext                                                  //
		// ---------------------------------------------------------------- //

		std::string MapExportContext::GetParam(const std::string& key) const {
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
		int MapExportContext::GetLayerCount() const {
			return mapParams ? (int)mapParams->layers.size() : 0;
		}
		std::string MapExportContext::GetLayerName(int layerIndex) const {
			if (!mapParams || layerIndex < 0 || layerIndex >= (int)mapParams->layers.size())
				return std::string();
			return mapParams->layers[layerIndex].name;
		}
		int MapExportContext::GetLayerWidth(int layerIndex) const {
			if (!mapParams || layerIndex < 0 || layerIndex >= (int)mapParams->layers.size())
				return 0;
			return mapParams->layers[layerIndex].width;
		}
		int MapExportContext::GetLayerHeight(int layerIndex) const {
			if (!mapParams || layerIndex < 0 || layerIndex >= (int)mapParams->layers.size())
				return 0;
			return mapParams->layers[layerIndex].height;
		}
		int MapExportContext::GetCell(int layerIndex, int col, int row) const {
			if (!mapParams || layerIndex < 0 || layerIndex >= (int)mapParams->layers.size())
				return 0;
			const MapLayer& layer = mapParams->layers[layerIndex];
			if (col < 0 || col >= layer.width || row < 0 || row >= layer.height)
				return 0;
			return (int)layer.data[row * layer.width + col];
		}
		int MapExportContext::GetCellTilesetIndex(int cellVal) const {
			if (cellVal == 0)
				return -1;
			return (int)(((uint16_t)cellVal >> 12) & 0xF) - 1;
		}
		int MapExportContext::GetCellTileIndex(int cellVal) const {
			return (int)((uint16_t)cellVal & 0x0FFF);
		}
		int MapExportContext::GetViewWidth() const {
			return mapParams ? mapParams->viewWidth : 0;
		}
		int MapExportContext::GetViewHeight() const {
			return mapParams ? mapParams->viewHeight : 0;
		}

		// ---------------------------------------------------------------- //
		// RegisterMapContextBinding                                         //
		// ---------------------------------------------------------------- //

		//
		// Generic wrappers -- required by AS_MAX_PORTABILITY builds
		//
		static void MapExportContext_GetLayerCount_Generic(asIScriptGeneric* gen) {
			gen->SetReturnDWord((asDWORD) static_cast<MapExportContext*>(gen->GetObject())->GetLayerCount());
		}
		static void MapExportContext_GetLayerName_Generic(asIScriptGeneric* gen) {
			int idx = gen->GetArgDWord(0);
			new (gen->GetAddressOfReturnLocation()) std::string(static_cast<MapExportContext*>(gen->GetObject())->GetLayerName(idx));
		}
		static void MapExportContext_GetLayerWidth_Generic(asIScriptGeneric* gen) {
			int idx = gen->GetArgDWord(0);
			gen->SetReturnDWord((asDWORD) static_cast<MapExportContext*>(gen->GetObject())->GetLayerWidth(idx));
		}
		static void MapExportContext_GetLayerHeight_Generic(asIScriptGeneric* gen) {
			int idx = gen->GetArgDWord(0);
			gen->SetReturnDWord((asDWORD) static_cast<MapExportContext*>(gen->GetObject())->GetLayerHeight(idx));
		}
		static void MapExportContext_GetCell_Generic(asIScriptGeneric* gen) {
			int layerIndex = gen->GetArgDWord(0);
			int col = gen->GetArgDWord(1);
			int row = gen->GetArgDWord(2);
			gen->SetReturnDWord((asDWORD) static_cast<MapExportContext*>(gen->GetObject())->GetCell(layerIndex, col, row));
		}
		static void MapExportContext_GetCellTilesetIndex_Generic(asIScriptGeneric* gen) {
			int cellVal = gen->GetArgDWord(0);
			gen->SetReturnDWord((asDWORD) static_cast<MapExportContext*>(gen->GetObject())->GetCellTilesetIndex(cellVal));
		}
		static void MapExportContext_GetCellTileIndex_Generic(asIScriptGeneric* gen) {
			int cellVal = gen->GetArgDWord(0);
			gen->SetReturnDWord((asDWORD) static_cast<MapExportContext*>(gen->GetObject())->GetCellTileIndex(cellVal));
		}
		static void MapExportContext_GetViewWidth_Generic(asIScriptGeneric* gen) {
			gen->SetReturnDWord((asDWORD) static_cast<MapExportContext*>(gen->GetObject())->GetViewWidth());
		}
		static void MapExportContext_GetViewHeight_Generic(asIScriptGeneric* gen) {
			gen->SetReturnDWord((asDWORD) static_cast<MapExportContext*>(gen->GetObject())->GetViewHeight());
		}
		static void MapExportContext_GetParam_Generic(asIScriptGeneric* gen) {
			const std::string& key = *static_cast<std::string*>(gen->GetArgObject(0));
			new (gen->GetAddressOfReturnLocation()) std::string(static_cast<MapExportContext*>(gen->GetObject())->GetParam(key));
		}
		//
		void RegisterMapContextBinding(asIScriptEngine* engine) {
			if (g_engine.mapContextRegistered)
				return;
			//
			// MapExportContext -- ref type, no script-side reference counting
			//
			engine->RegisterObjectType("MapExportContext", 0, asOBJ_REF | asOBJ_NOCOUNT);
			engine->RegisterObjectMethod("MapExportContext", "int GetLayerCount() const", asFUNCTION(MapExportContext_GetLayerCount_Generic), asCALL_GENERIC);
			engine->RegisterObjectMethod("MapExportContext", "string GetLayerName(int layerIndex) const", asFUNCTION(MapExportContext_GetLayerName_Generic), asCALL_GENERIC);
			engine->RegisterObjectMethod("MapExportContext", "int GetLayerWidth(int layerIndex) const", asFUNCTION(MapExportContext_GetLayerWidth_Generic), asCALL_GENERIC);
			engine->RegisterObjectMethod("MapExportContext", "int GetLayerHeight(int layerIndex) const", asFUNCTION(MapExportContext_GetLayerHeight_Generic), asCALL_GENERIC);
			engine->RegisterObjectMethod("MapExportContext", "int GetCell(int layerIndex, int col, int row) const", asFUNCTION(MapExportContext_GetCell_Generic), asCALL_GENERIC);
			engine->RegisterObjectMethod("MapExportContext", "int GetCellTilesetIndex(int cellVal) const", asFUNCTION(MapExportContext_GetCellTilesetIndex_Generic),
										 asCALL_GENERIC);
			engine->RegisterObjectMethod("MapExportContext", "int GetCellTileIndex(int cellVal) const", asFUNCTION(MapExportContext_GetCellTileIndex_Generic), asCALL_GENERIC);
			engine->RegisterObjectMethod("MapExportContext", "int GetViewWidth() const", asFUNCTION(MapExportContext_GetViewWidth_Generic), asCALL_GENERIC);
			engine->RegisterObjectMethod("MapExportContext", "int GetViewHeight() const", asFUNCTION(MapExportContext_GetViewHeight_Generic), asCALL_GENERIC);
			engine->RegisterObjectMethod("MapExportContext", "string GetParam(const string &in) const", asFUNCTION(MapExportContext_GetParam_Generic), asCALL_GENERIC);
			g_engine.mapContextRegistered = true;
		}

		// ---------------------------------------------------------------- //
		// RunMapExport                                                      //
		// ---------------------------------------------------------------- //

		bool RunMapExport(const std::string& scriptPath, const std::string& outputPath, const std::string& scriptParams, const MapParams* mapParams) {
			if (!g_engine.initialized) {
				ReportError("[Script] ExportMap: engine not initialized -- call ExportEngine::Initialize() first");
				return false;
			}
			if (!EnsureOutputDirectory(outputPath))
				return false;
			//
			// Reset error flag so a Log_Error() call inside the script is detectable after execute
			//
			g_hasError = false;
			//
			// Register all bindings required by map export scripts
			//
			RegisterAddons(g_engine.engine);
			RegisterMapContextBinding(g_engine.engine);
			//
			// Load, compile and build the script into a transient module
			//
			const std::string moduleName = "ExportMap";
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
				ReportError("[Script] ExportMap: module not found after build");
				g_engine.Discard(moduleName);
				return false;
			}
			const std::string funcDecl = "void Export(const string &in, MapExportContext@)";
			asIScriptFunction* func = mod->GetFunctionByDecl(funcDecl.c_str());
			if (func == nullptr) {
				ReportError("[Script] ExportMap: function '" + funcDecl + "' not found in script '" + scriptPath + "'");
				g_engine.Discard(moduleName);
				return false;
			}
			//
			// Build the context object and invoke the script function
			//
			MapExportContext exportCtx;
			exportCtx.mapParams = mapParams;
			exportCtx.scriptParams = scriptParams;
			asIScriptContext* ctx = g_engine.engine->CreateContext();
			if (ctx == nullptr) {
				ReportError("[Script] ExportMap: failed to create script context");
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
				ReportError("[Script] ExportMap: execution did not finish normally (r=" + std::to_string(r) + ")");
				return false;
			}
			if (g_hasError)
				return false;
			return true;
		}

	}
}
