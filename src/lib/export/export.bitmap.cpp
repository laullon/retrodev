// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Bitmap export engine -- runs AngelScript export scripts for bitmap items.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#include "export.bitmap.h"
#include <assets/image/image.h>
#include <assets/image/image.color.h>
#include <convert/convert.bitmap.h>
#include <convert/convert.bitmap.params.h>
#include <log/log.h>

namespace RetrodevLib {
	namespace ExportImpl {

		// -------------------------------------------------------------- //
		// BitmapExportContext                                              //
		// -------------------------------------------------------------- //

		std::string BitmapExportContext::GetParam(const std::string& key) const {
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
		int BitmapExportContext::GetNativeWidth() const {
			return (converter && params) ? converter->GetNativeWidth(params) : 0;
		}
		int BitmapExportContext::GetNativeHeight() const {
			return (converter && params) ? converter->GetNativeHeight(params) : 0;
		}
		std::string BitmapExportContext::GetTargetMode() const {
			return params ? params->SParams.TargetMode : std::string();
		}
		std::string BitmapExportContext::GetTargetSystem() const {
			return params ? params->SParams.TargetSystem : std::string();
		}
		IPaletteConverter* BitmapExportContext::GetPalette() const {
			return converter ? converter->GetPalette().get() : nullptr;
		}
		bool BitmapExportContext::GetUseTransparentColor() const {
			return params ? params->RParams.UseTransparentColor : false;
		}
		int BitmapExportContext::GetTransparentPen() const {
			return params ? params->RParams.TransparentPen : -1;
		}

		// -------------------------------------------------------------- //
		// RegisterBitmapContextBinding                                     //
		// -------------------------------------------------------------- //

		//
		// Generic wrappers -- required by AS_MAX_PORTABILITY builds
		//
		static void BitmapExportContext_GetNativeWidth_Generic(asIScriptGeneric* gen) {
			gen->SetReturnDWord((asDWORD) static_cast<BitmapExportContext*>(gen->GetObject())->GetNativeWidth());
		}
		static void BitmapExportContext_GetNativeHeight_Generic(asIScriptGeneric* gen) {
			gen->SetReturnDWord((asDWORD) static_cast<BitmapExportContext*>(gen->GetObject())->GetNativeHeight());
		}
		static void BitmapExportContext_GetTargetMode_Generic(asIScriptGeneric* gen) {
			new (gen->GetAddressOfReturnLocation()) std::string(static_cast<BitmapExportContext*>(gen->GetObject())->GetTargetMode());
		}
		static void BitmapExportContext_GetTargetSystem_Generic(asIScriptGeneric* gen) {
			new (gen->GetAddressOfReturnLocation()) std::string(static_cast<BitmapExportContext*>(gen->GetObject())->GetTargetSystem());
		}
		static void BitmapExportContext_GetParam_Generic(asIScriptGeneric* gen) {
			const std::string& key = *static_cast<std::string*>(gen->GetArgObject(0));
			new (gen->GetAddressOfReturnLocation()) std::string(static_cast<BitmapExportContext*>(gen->GetObject())->GetParam(key));
		}
		static void BitmapExportContext_GetPalette_Generic(asIScriptGeneric* gen) {
			gen->SetReturnAddress(static_cast<BitmapExportContext*>(gen->GetObject())->GetPalette());
		}
		static void BitmapExportContext_GetUseTransparentColor_Generic(asIScriptGeneric* gen) {
			gen->SetReturnByte(static_cast<BitmapExportContext*>(gen->GetObject())->GetUseTransparentColor() ? 1 : 0);
		}
		static void BitmapExportContext_GetTransparentPen_Generic(asIScriptGeneric* gen) {
			gen->SetReturnDWord((asDWORD) static_cast<BitmapExportContext*>(gen->GetObject())->GetTransparentPen());
		}
		//
		void RegisterBitmapContextBinding(asIScriptEngine* engine) {
			if (g_engine.bitmapContextRegistered)
				return;
			//
			// BitmapExportContext -- ref type, no script-side reference counting
			//
			engine->RegisterObjectType("BitmapExportContext", 0, asOBJ_REF | asOBJ_NOCOUNT);
			engine->RegisterObjectMethod("BitmapExportContext", "int GetNativeWidth() const", asFUNCTION(BitmapExportContext_GetNativeWidth_Generic), asCALL_GENERIC);
			engine->RegisterObjectMethod("BitmapExportContext", "int GetNativeHeight() const", asFUNCTION(BitmapExportContext_GetNativeHeight_Generic), asCALL_GENERIC);
			engine->RegisterObjectMethod("BitmapExportContext", "string GetTargetMode() const", asFUNCTION(BitmapExportContext_GetTargetMode_Generic), asCALL_GENERIC);
			engine->RegisterObjectMethod("BitmapExportContext", "string GetTargetSystem() const", asFUNCTION(BitmapExportContext_GetTargetSystem_Generic), asCALL_GENERIC);
			engine->RegisterObjectMethod("BitmapExportContext", "string GetParam(const string &in) const", asFUNCTION(BitmapExportContext_GetParam_Generic), asCALL_GENERIC);
			engine->RegisterObjectMethod("BitmapExportContext", "Palette@ GetPalette() const", asFUNCTION(BitmapExportContext_GetPalette_Generic), asCALL_GENERIC);
			engine->RegisterObjectMethod("BitmapExportContext", "bool GetUseTransparentColor() const", asFUNCTION(BitmapExportContext_GetUseTransparentColor_Generic), asCALL_GENERIC);
			engine->RegisterObjectMethod("BitmapExportContext", "int GetTransparentPen() const", asFUNCTION(BitmapExportContext_GetTransparentPen_Generic), asCALL_GENERIC);
			g_engine.bitmapContextRegistered = true;
		}

		// -------------------------------------------------------------- //
		// RunBitmapExport                                                  //
		// -------------------------------------------------------------- //

		bool RunBitmapExport(const std::string& scriptPath, const std::string& outputPath, const std::string& scriptParams, Image* image, IBitmapConverter* converter,
							 const GFXParams* params) {
			if (!g_engine.initialized) {
				ReportError("[Script] ExportBitmap: engine not initialized -- call ExportEngine::Initialize() first");
				return false;
			}
			if (!EnsureOutputDirectory(outputPath))
				return false;
			//
			// Register all bindings required by bitmap export scripts
			//
			RegisterRgbColorBinding(g_engine.engine);
			RegisterImageBinding(g_engine.engine);
			RegisterPaletteBinding(g_engine.engine);
			RegisterBitmapContextBinding(g_engine.engine);
			//
			// Load, compile and build the script into a transient module
			//
			const std::string moduleName = "ExportBitmap";
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
				ReportError("[Script] ExportBitmap: module not found after build");
				g_engine.Discard(moduleName);
				return false;
			}
			const std::string funcDecl = "void Export(Image@, const string &in, BitmapExportContext@)";
			asIScriptFunction* func = mod->GetFunctionByDecl(funcDecl.c_str());
			if (func == nullptr) {
				ReportError("[Script] ExportBitmap: function '" + funcDecl + "' not found in script '" + scriptPath + "'");
				g_engine.Discard(moduleName);
				return false;
			}
			//
			// Build the context object and invoke the script function
			//
			BitmapExportContext exportCtx;
			exportCtx.converter = converter;
			exportCtx.params = params;
			exportCtx.scriptParams = scriptParams;
			asIScriptContext* ctx = g_engine.engine->CreateContext();
			if (ctx == nullptr) {
				ReportError("[Script] ExportBitmap: failed to create script context");
				g_engine.Discard(moduleName);
				return false;
			}
			ctx->Prepare(func);
			ctx->SetArgObject(0, image);
			ctx->SetArgObject(1, const_cast<std::string*>(&outputPath));
			ctx->SetArgObject(2, &exportCtx);
			int r = ctx->Execute();
			ctx->Release();
			//
			// Always discard so the next call reloads the script from disk
			//
			g_engine.Discard(moduleName);
			if (r != asEXECUTION_FINISHED) {
				ReportError("[Script] ExportBitmap: execution did not finish normally (r=" + std::to_string(r) + ")");
				return false;
			}
			return true;
		}

	}
}