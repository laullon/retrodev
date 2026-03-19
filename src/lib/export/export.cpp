// --------------------------------------------------------------------------------------------------------------
//
//
//
//
// --------------------------------------------------------------------------------------------------------------

#include "export.h"
#include "export.engine.h"
#include "export.bitmap.h"
#include "export.tileset.h"
#include "export.sprites.h"
#include "export.map.h"

namespace RetrodevLib {

	bool ExportEngine::Initialize() {
		return ExportImpl::g_engine.Initialize();
	}
	void ExportEngine::Shutdown() {
		ExportImpl::g_engine.Shutdown();
		ExportImpl::g_errors.clear();
		ExportImpl::g_hasError = false;
	}
	bool ExportEngine::HasErrors() {
		return ExportImpl::g_hasError;
	}
	const std::vector<std::string>& ExportEngine::GetErrors() {
		return ExportImpl::g_errors;
	}
	void ExportEngine::ClearErrors() {
		ExportImpl::g_errors.clear();
		ExportImpl::g_hasError = false;
	}
	ScriptMetadata ExportEngine::GetScriptMetadata(const std::string& scriptPath) {
		return ExportImpl::ScriptEngine::GetScriptMetadata(scriptPath);
	}
	std::string ExportEngine::GetScriptDescription(const std::string& scriptPath) {
		return ExportImpl::ScriptEngine::GetScriptDescription(scriptPath);
	}
	bool ExportEngine::ExportBitmap(const std::string& scriptPath, const std::string& outputPath, const std::string& scriptParams, Image* image, IBitmapConverter* converter,
									const GFXParams* params) {
		return ExportImpl::RunBitmapExport(scriptPath, outputPath, scriptParams, image, converter, params);
	}
	bool ExportEngine::ExportTileset(const std::string& scriptPath, const std::string& outputPath, const std::string& scriptParams, IBitmapConverter* converter,
									 const GFXParams* params, ITileExtractor* tileExtractor, const TileExtractionParams* tileParams) {
		return ExportImpl::RunTilesetExport(scriptPath, outputPath, scriptParams, converter, params, tileExtractor, tileParams);
	}
	bool ExportEngine::ExportSprites(const std::string& scriptPath, const std::string& outputPath, const std::string& scriptParams, IBitmapConverter* converter,
									 const GFXParams* params, ISpriteExtractor* spriteExtractor, const SpriteExtractionParams* spriteParams) {
		return ExportImpl::RunSpriteExport(scriptPath, outputPath, scriptParams, converter, params, spriteExtractor, spriteParams);
	}
	bool ExportEngine::ExportMap(const std::string& scriptPath, const std::string& outputPath, const std::string& scriptParams, const MapParams* mapParams) {
		return ExportImpl::RunMapExport(scriptPath, outputPath, scriptParams, mapParams);
	}
} 
