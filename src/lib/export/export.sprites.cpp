// --------------------------------------------------------------------------------------------------------------
//
//
//
//
// --------------------------------------------------------------------------------------------------------------

#include "export.sprites.h"
#include <assets/image/image.h>
#include <assets/image/image.color.h>
#include <convert/convert.bitmap.h>
#include <convert/convert.bitmap.params.h>
#include <log/log.h>

namespace RetrodevLib {
	namespace ExportImpl {

		// ---------------------------------------------------------------- //
		// SpriteExportContext                                               //
		// ---------------------------------------------------------------- //

		std::string SpriteExportContext::GetParam(const std::string& key) const {
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
		int SpriteExportContext::GetSpriteCount() const {
			return spriteExtractor ? spriteExtractor->GetSpriteCount() : 0;
		}
		Image* SpriteExportContext::GetSprite(int index) const {
			if (!spriteExtractor)
				return nullptr;
			auto sprite = spriteExtractor->GetSprite(index);
			return sprite ? sprite.get() : nullptr;
		}
		int SpriteExportContext::GetSpriteWidth(int index) const {
			if (!spriteExtractor)
				return 0;
			const SpriteDefinition* def = spriteExtractor->GetSpriteDefinition(index);
			return def ? def->Width : 0;
		}
		int SpriteExportContext::GetSpriteHeight(int index) const {
			if (!spriteExtractor)
				return 0;
			const SpriteDefinition* def = spriteExtractor->GetSpriteDefinition(index);
			return def ? def->Height : 0;
		}
		std::string SpriteExportContext::GetSpriteName(int index) const {
			if (!spriteExtractor)
				return std::string();
			const SpriteDefinition* def = spriteExtractor->GetSpriteDefinition(index);
			return def ? def->Name : std::string();
		}
		int SpriteExportContext::GetNativeWidth() const {
			return (converter && params) ? converter->GetNativeWidth(params) : 0;
		}
		int SpriteExportContext::GetNativeHeight() const {
			return (converter && params) ? converter->GetNativeHeight(params) : 0;
		}
		std::string SpriteExportContext::GetTargetMode() const {
			return params ? params->SParams.TargetMode : std::string();
		}
		std::string SpriteExportContext::GetTargetSystem() const {
			return params ? params->SParams.TargetSystem : std::string();
		}
		IPaletteConverter* SpriteExportContext::GetPalette() const {
			return converter ? converter->GetPalette().get() : nullptr;
		}

		// ---------------------------------------------------------------- //
		// RegisterSpriteContextBinding                                      //
		// ---------------------------------------------------------------- //

		//
		// Generic wrappers — required by AS_MAX_PORTABILITY builds
		//
		static void SpriteExportContext_GetSpriteCount_Generic(asIScriptGeneric* gen) {
			gen->SetReturnDWord((asDWORD) static_cast<SpriteExportContext*>(gen->GetObject())->GetSpriteCount());
		}
		static void SpriteExportContext_GetSprite_Generic(asIScriptGeneric* gen) {
			int index = gen->GetArgDWord(0);
			gen->SetReturnAddress(static_cast<SpriteExportContext*>(gen->GetObject())->GetSprite(index));
		}
		static void SpriteExportContext_GetSpriteWidth_Generic(asIScriptGeneric* gen) {
			int index = gen->GetArgDWord(0);
			gen->SetReturnDWord((asDWORD) static_cast<SpriteExportContext*>(gen->GetObject())->GetSpriteWidth(index));
		}
		static void SpriteExportContext_GetSpriteHeight_Generic(asIScriptGeneric* gen) {
			int index = gen->GetArgDWord(0);
			gen->SetReturnDWord((asDWORD) static_cast<SpriteExportContext*>(gen->GetObject())->GetSpriteHeight(index));
		}
		static void SpriteExportContext_GetSpriteName_Generic(asIScriptGeneric* gen) {
			int index = gen->GetArgDWord(0);
			new (gen->GetAddressOfReturnLocation()) std::string(static_cast<SpriteExportContext*>(gen->GetObject())->GetSpriteName(index));
		}
		static void SpriteExportContext_GetNativeWidth_Generic(asIScriptGeneric* gen) {
			gen->SetReturnDWord((asDWORD) static_cast<SpriteExportContext*>(gen->GetObject())->GetNativeWidth());
		}
		static void SpriteExportContext_GetNativeHeight_Generic(asIScriptGeneric* gen) {
			gen->SetReturnDWord((asDWORD) static_cast<SpriteExportContext*>(gen->GetObject())->GetNativeHeight());
		}
		static void SpriteExportContext_GetTargetMode_Generic(asIScriptGeneric* gen) {
			new (gen->GetAddressOfReturnLocation()) std::string(static_cast<SpriteExportContext*>(gen->GetObject())->GetTargetMode());
		}
		static void SpriteExportContext_GetTargetSystem_Generic(asIScriptGeneric* gen) {
			new (gen->GetAddressOfReturnLocation()) std::string(static_cast<SpriteExportContext*>(gen->GetObject())->GetTargetSystem());
		}
		static void SpriteExportContext_GetParam_Generic(asIScriptGeneric* gen) {
			const std::string& key = *static_cast<std::string*>(gen->GetArgObject(0));
			new (gen->GetAddressOfReturnLocation()) std::string(static_cast<SpriteExportContext*>(gen->GetObject())->GetParam(key));
		}
		static void SpriteExportContext_GetPalette_Generic(asIScriptGeneric* gen) {
			gen->SetReturnAddress(static_cast<SpriteExportContext*>(gen->GetObject())->GetPalette());
		}
		//
		void RegisterSpriteContextBinding(asIScriptEngine* engine) {
			if (g_engine.spriteContextRegistered)
				return;
			//
			// SpriteExportContext — ref type, no script-side reference counting
			//
			engine->RegisterObjectType("SpriteExportContext", 0, asOBJ_REF | asOBJ_NOCOUNT);
			engine->RegisterObjectMethod("SpriteExportContext", "int GetSpriteCount() const", asFUNCTION(SpriteExportContext_GetSpriteCount_Generic), asCALL_GENERIC);
			engine->RegisterObjectMethod("SpriteExportContext", "Image@ GetSprite(int index) const", asFUNCTION(SpriteExportContext_GetSprite_Generic), asCALL_GENERIC);
			engine->RegisterObjectMethod("SpriteExportContext", "int GetSpriteWidth(int index) const", asFUNCTION(SpriteExportContext_GetSpriteWidth_Generic), asCALL_GENERIC);
			engine->RegisterObjectMethod("SpriteExportContext", "int GetSpriteHeight(int index) const", asFUNCTION(SpriteExportContext_GetSpriteHeight_Generic), asCALL_GENERIC);
			engine->RegisterObjectMethod("SpriteExportContext", "string GetSpriteName(int index) const", asFUNCTION(SpriteExportContext_GetSpriteName_Generic), asCALL_GENERIC);
			engine->RegisterObjectMethod("SpriteExportContext", "int GetNativeWidth() const", asFUNCTION(SpriteExportContext_GetNativeWidth_Generic), asCALL_GENERIC);
			engine->RegisterObjectMethod("SpriteExportContext", "int GetNativeHeight() const", asFUNCTION(SpriteExportContext_GetNativeHeight_Generic), asCALL_GENERIC);
			engine->RegisterObjectMethod("SpriteExportContext", "string GetTargetMode() const", asFUNCTION(SpriteExportContext_GetTargetMode_Generic), asCALL_GENERIC);
			engine->RegisterObjectMethod("SpriteExportContext", "string GetTargetSystem() const", asFUNCTION(SpriteExportContext_GetTargetSystem_Generic), asCALL_GENERIC);
			engine->RegisterObjectMethod("SpriteExportContext", "string GetParam(const string &in) const", asFUNCTION(SpriteExportContext_GetParam_Generic), asCALL_GENERIC);
			engine->RegisterObjectMethod("SpriteExportContext", "Palette@ GetPalette() const", asFUNCTION(SpriteExportContext_GetPalette_Generic), asCALL_GENERIC);
			g_engine.spriteContextRegistered = true;
		}

		// ---------------------------------------------------------------- //
		// RunSpriteExport                                                   //
		// ---------------------------------------------------------------- //

		bool RunSpriteExport(const std::string& scriptPath, const std::string& outputPath, const std::string& scriptParams, IBitmapConverter* converter, const GFXParams* params,
						 ISpriteExtractor* spriteExtractor, const SpriteExtractionParams* spriteParams) {
			if (!g_engine.initialized) {
				ReportError("[Script] ExportSprites: engine not initialized — call ExportEngine::Initialize() first");
				return false;
			}
			if (!EnsureOutputDirectory(outputPath))
				return false;
			//
			// Register all bindings required by sprite export scripts
			//
			RegisterRgbColorBinding(g_engine.engine);
			RegisterImageBinding(g_engine.engine);
			RegisterPaletteBinding(g_engine.engine);
			RegisterSpriteContextBinding(g_engine.engine);
			//
			// Load, compile and build the script into a transient module
			//
			const std::string moduleName = "ExportSprites";
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
				ReportError("[Script] ExportSprites: module not found after build");
				g_engine.Discard(moduleName);
				return false;
			}
			const std::string funcDecl = "void Export(const string &in, SpriteExportContext@)";
			asIScriptFunction* func = mod->GetFunctionByDecl(funcDecl.c_str());
			if (func == nullptr) {
				ReportError("[Script] ExportSprites: function '" + funcDecl + "' not found in script '" + scriptPath + "'");
				g_engine.Discard(moduleName);
				return false;
			}
			//
			// Build the context object and invoke the script function
			//
			SpriteExportContext exportCtx;
			exportCtx.converter = converter;
			exportCtx.params = params;
			exportCtx.spriteExtractor = spriteExtractor;
			exportCtx.spriteParams = spriteParams;
			exportCtx.scriptParams = scriptParams;
			asIScriptContext* ctx = g_engine.engine->CreateContext();
			if (ctx == nullptr) {
				ReportError("[Script] ExportSprites: failed to create script context");
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
				ReportError("[Script] ExportSprites: execution did not finish normally (r=" + std::to_string(r) + ")");
				return false;
			}
			return true;
		}

	} // namespace ExportImpl
} // namespace RetrodevLib
