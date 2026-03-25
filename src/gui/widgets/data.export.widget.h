// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Gui
//
// Data export widget -- export script selection and parameter UI.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <retrodev.gui.h>
#include <retrodev.lib.h>
#include <project/project.h>
#include <convert/converters.h>
#include <convert/convert.tileset.h>
#include <convert/convert.tileset.params.h>
#include <convert/convert.sprites.h>
#include <convert/convert.sprites.params.h>
#include <export/export.h>
#include <memory>
#include <string>
#include <vector>

namespace RetrodevGui {

	//
	// DataExportWidget -- script-driven export UI, usable by any document type
	//
	// Each document that needs export must own one instance so path buffers
	// and collapsing-header state are not shared across simultaneous documents.
	//
	class DataExportWidget {
	public:
		//
		// Render the export section.
		// buildType + buildName identify the owning build item so ExportParams can be
		// fetched, mutated in-place and saved back via MarkAsModified().
		// tileExtractor / tileParams are optional -- supply them for tileset documents;
		// leave them null for bitmap / sprite / map documents (default).
		//
		void Render(RetrodevLib::ProjectBuildType buildType, const std::string& buildName, RetrodevLib::GFXParams* params, std::shared_ptr<RetrodevLib::IBitmapConverter> converter,
					std::shared_ptr<RetrodevLib::ITileExtractor> tileExtractor = nullptr, RetrodevLib::TileExtractionParams* tileParams = nullptr,
					std::shared_ptr<RetrodevLib::ISpriteExtractor> spriteExtractor = nullptr, RetrodevLib::SpriteExtractionParams* spriteParams = nullptr);
		//
		// Render the export section for a map document.
		// Takes MapParams directly -- no converter or GFXParams required.
		//
		void RenderMap(const std::string& buildName, const RetrodevLib::MapParams* mapParams);

	private:
		//
		// Renders the script picker modal. Called unconditionally every frame from Render()
		// so OpenPopup and BeginPopupModal share the same ID stack context.
		//
		void RenderScriptPicker();
		//
		// Dispatch the actual export call based on which context data is available.
		// Called only when canExport is true and the user clicks the Export button.
		//
		bool RunExport(RetrodevLib::GFXParams* params, std::shared_ptr<RetrodevLib::IBitmapConverter> converter, std::shared_ptr<RetrodevLib::ITileExtractor> tileExtractor,
					   RetrodevLib::TileExtractionParams* tileParams, std::shared_ptr<RetrodevLib::ISpriteExtractor> spriteExtractor,
					   RetrodevLib::SpriteExtractionParams* spriteParams, const std::string& absoluteScript, const std::string& absoluteOutput);
		//
		// Renders the dynamic parameter controls built from m_paramDefs.
		// Reads and writes values through m_exportParams->scriptParams.
		//
		void RenderParamControls();
		//
		// Populate m_paramDefs from the given script metadata and ensure
		// m_exportParams->scriptParams contains a value for every declared key,
		// inserting the default for any key that is missing.
		//
		void SyncParamDefs(const std::vector<RetrodevLib::ScriptParamDef>& defs);
		//
		// Per-instance state -- picker modal flags and metadata cache only.
		// Path data lives in the project's ExportParams, not here.
		//
		bool m_outputOpen = true;
		bool m_showScriptPicker = false;
		int m_pickerSelectedIndex = -1;
		//
		// Pointer to the active build item's ExportParams, valid for the duration of Render().
		// Used by RenderScriptPicker() and RenderParamControls() to read/write the project.
		//
		RetrodevLib::ExportParams* m_exportParams = nullptr;
		//
		// Script path for which m_paramDefs was last populated.
		// When it differs from m_exportParams->scriptPath the defs are refreshed.
		//
		std::string m_paramDefsScript;
		//
		// Cached parameter definitions for the currently selected script.
		// Rebuilt only when the script path changes.
		//
		std::vector<RetrodevLib::ScriptParamDef> m_paramDefs;
		//
		// Metadata cache -- populated once when the picker opens, cleared on close.
		// Avoids reading script files every frame inside the render loop.
		//
		struct PickerEntry {
			std::string path;
			std::string filename;
			std::string folder;
			std::string description;
			std::string exporter;
			std::string target;
			std::vector<RetrodevLib::ScriptParamDef> params;
		};
		std::vector<PickerEntry> m_pickerEntries;
		//
		// Active filter values set at the start of Render() and used by RenderScriptPicker()
		// to skip scripts whose @exporter or @target do not match the active document.
		// An empty string means "no filter applied" for that field.
		//
		std::string m_activeSystemId;
		std::string m_activeBuildTypeTag;
	};

}
