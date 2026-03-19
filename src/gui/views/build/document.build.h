// --------------------------------------------------------------------------------------------------------------
//
//
//
//
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <views/main.view.document.h>
#include <retrodev.gui.h>
#include <retrodev.lib.h>
#include <views/build/document.build.rasm.h>
#include <views/build/document.build.settings.h>
#include <string>

namespace RetrodevGui {

	class DocumentBuild : public DocumentView {
	public:
		DocumentBuild(const std::string& name);
		~DocumentBuild() override;
		//
		// Render the build document content
		//
		void Perform() override;
		//
		// Get the build type this document represents
		//
		RetrodevLib::ProjectBuildType GetBuildType() const override { return RetrodevLib::ProjectBuildType::Build; }

	private:
		//
		// Active tab index (0=Source, 1=Build, 2=Output, 3=Debug)
		//
		// int m_activeTab = 0;
		//
		// Index of the selected item in the Sources list (-1 = none)
		//
		int m_selectedSourceIdx = -1;
		//
		// Index of the selected item in the Include Dirs list (-1 = none)
		//
		int m_selectedIncludeDirIdx = -1;
		//
		// Index of the selected item in the Defines list (-1 = none)
		//
		int m_selectedDefineIdx = -1;
		//
		// Index of the selected item in the Dependencies list (-1 = none)
		//
		int m_selectedDependencyIdx = -1;
		//
		// Input buffer for add-define
		//
		char m_defineAddBuf[256] = {};
		//
		// Render the Source tab (sources, include dirs, defines)
		//
		void RenderTabSource(RetrodevLib::SourceParams* params);
		//
		// RASM-specific panel renderer
		//
		DocumentBuildRasm m_rasm;
		//
		// Render the Build tab (tool selection and tool-specific build options)
		//
		void RenderTabBuild(RetrodevLib::SourceParams* params);
		//
		// Render the Output tab (output format options for the selected tool)
		//
		void RenderTabOutput(RetrodevLib::SourceParams* params);
		//
		// Render the Debug tab (symbol generation and debug options for the selected tool)
		//
		void RenderTabDebug(RetrodevLib::SourceParams* params);
		//
		// Render the common emulator launch section at the bottom of the Debug tab
		//
		void RenderTabDebugCommon(RetrodevLib::SourceParams* params);
		//
		// Render the Sources sub-panel (used by RenderTabSource)
		//
		void RenderSources(RetrodevLib::SourceParams* params);
		//
		// Render the Include Directories sub-panel (used by RenderTabSource)
		//
		void RenderIncludeDirs(RetrodevLib::SourceParams* params);
		//
		// Render the Defines sub-panel (used by RenderTabSource)
		//
		void RenderDefines(RetrodevLib::SourceParams* params);
		//
		// Render the Dependencies sub-panel (used by RenderTabSource)
		//
		void RenderDependencies(RetrodevLib::SourceParams* params);
		//
		// Persistent left-panel width for the Source tab horizontal splitter (0 = uninitialised)
		//
		float m_srcTabLeftW = 0.0f;
	};

} 