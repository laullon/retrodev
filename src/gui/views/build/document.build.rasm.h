// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Gui
//
// Build document -- RASM assembler invocation and output parsing.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <retrodev.gui.h>
#include <retrodev.lib.h>

namespace RetrodevGui {

	//
	// RASM-specific UI panels for the Build document.
	// Owns no project state -- operates exclusively on the SourceParams pointer
	// supplied by DocumentBuild on each Perform() call.
	//
	class DocumentBuildRasm {
	public:
		//
		// Render the Build sub-panel: warnings/errors, compatibility, macro behaviour
		//
		void RenderBuildOptions(RetrodevLib::SourceParams* params);
		//
		// Render the Output sub-panel: output format options (placeholder)
		//
		void RenderOutputOptions(RetrodevLib::SourceParams* params);
		//
		// Render the Debug sub-panel: symbol generation and debug options (placeholder)
		//
		void RenderDebugOptions(RetrodevLib::SourceParams* params);
		//
		// Bind the owning document's modified flag so every change marks the tab dirty
		//
		void SetModifiedFlag(bool* flag) { m_docModified = flag; }

	private:
		//
		// Returns true if the flag token is present as a whole word in opts
		//
		static bool HasFlag(const std::string& opts, const std::string& flag);
		//
		// Returns the value following a flag token (e.g. "-me 5" -> "5"), empty if absent
		//
		static std::string GetFlagValue(const std::string& opts, const std::string& flag);
		//
		// Sets or clears a flag token in opts
		//
		static void SetFlag(std::string& opts, const std::string& flag, bool on);
		//
		// Sets or replaces a flag+value token (e.g. "-me 5") in opts;
		// passing an empty value removes the flag entirely
		//
		static void SetFlagValue(std::string& opts, const std::string& flag, const std::string& value);
		//
		// Render a single path input row tied to a flag+value token.
		// label: display label shown next to the field.
		// flag: CLI flag stored in opts (e.g. "-ob").
		// buf / bufSize: per-row char buffer kept as a member (synced from opts each frame).
		// hint: placeholder text shown when the field is empty (pass nullptr for none).
		// opts: the CLI string to read from and write to.
		//
		void RenderPathField(const char* label, const char* flag, char* buf, size_t bufSize, std::string& opts, const char* hint = nullptr);
		//
		// Per-field path input buffers (synced from toolOptions on every frame)
		//
		char m_outputRadixBuf[512] = {};
		char m_binaryNameBuf[512] = {};
		char m_romNameBuf[512] = {};
		char m_cartridgeNameBuf[512] = {};
		char m_snapshotNameBuf[512] = {};
		char m_tapeNameBuf[512] = {};
		char m_symbolNameBuf[512] = {};
		char m_breakpointNameBuf[512] = {};
		char m_cprinfoNameBuf[512] = {};
		char m_flexibleExportBuf[512] = {};
		//
		// Pointer to the owning document's modified flag; set by DocumentBuild before rendering.
		// Every MarkAsModified() call in this class also sets *m_docModified = true.
		//
		bool* m_docModified = nullptr;
	};

}
