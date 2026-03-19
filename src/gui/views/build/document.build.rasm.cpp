// --------------------------------------------------------------------------------------------------------------
//
//
//
//
//
// --------------------------------------------------------------------------------------------------------------

#include "document.build.rasm.h"
#include <app/app.icons.mdi.h>
#include <cstdlib>

namespace RetrodevGui {
	//
	// Splits opts into tokens respecting flag+value pairs.
	// Tokens are alternating: flag, optional-value, flag, optional-value ...
	// A token starting with '-' is a flag; the next token is its value if it does not start with '-'.
	// Returns a list of (tokenText, isFlagStart) pairs where isFlagStart marks flag tokens.
	// This tokeniser is the single source of truth used by HasFlag, GetFlagValue, SetFlag, SetFlagValue
	// so that value strings containing flag-like substrings never cause false matches.
	//
	static void TokenizeOpts(const std::string& opts, std::vector<std::string>& outTokens) {
		outTokens.clear();
		size_t i = 0;
		const size_t n = opts.size();
		while (i < n) {
			while (i < n && opts[i] == ' ')
				i++;
			if (i >= n)
				break;
			size_t start = i;
			while (i < n && opts[i] != ' ')
				i++;
			outTokens.push_back(opts.substr(start, i - start));
		}
	}
	//
	// Returns true if flag is present as a standalone flag token in opts.
	// Only flag tokens (those starting with '-') are checked; value tokens are skipped,
	// preventing a value string like "out/program.rasm" from matching "-rasm".
	//
	bool DocumentBuildRasm::HasFlag(const std::string& opts, const std::string& flag) {
		std::vector<std::string> tokens;
		TokenizeOpts(opts, tokens);
		for (size_t i = 0; i < tokens.size(); i++) {
			if (tokens[i][0] != '-')
				continue;
			if (tokens[i] == flag)
				return true;
		}
		return false;
	}
	//
	// Returns the value following a flag token (e.g. "-me 5" -> "5"), empty if absent.
	// Only examines actual flag tokens so values are never confused with flags.
	//
	std::string DocumentBuildRasm::GetFlagValue(const std::string& opts, const std::string& flag) {
		std::vector<std::string> tokens;
		TokenizeOpts(opts, tokens);
		for (size_t i = 0; i < tokens.size(); i++) {
			if (tokens[i][0] != '-')
				continue;
			if (tokens[i] == flag) {
				if (i + 1 < tokens.size() && tokens[i + 1][0] != '-')
					return tokens[i + 1];
				return "";
			}
		}
		return "";
	}
	//
	// Sets or clears a flag token in opts.
	// Rebuilds opts from tokens to guarantee clean formatting and avoid stale duplicates.
	//
	void DocumentBuildRasm::SetFlag(std::string& opts, const std::string& flag, bool on) {
		std::vector<std::string> tokens;
		TokenizeOpts(opts, tokens);
		bool found = false;
		for (size_t i = 0; i < tokens.size(); i++) {
			if (tokens[i][0] != '-')
				continue;
			if (tokens[i] == flag) {
				found = true;
				if (!on) {
					tokens.erase(tokens.begin() + (int)i);
					if (i < tokens.size() && tokens[i][0] != '-')
						tokens.erase(tokens.begin() + (int)i);
				}
				break;
			}
		}
		if (on && !found)
			tokens.push_back(flag);
		opts.clear();
		for (size_t i = 0; i < tokens.size(); i++) {
			if (i > 0)
				opts += ' ';
			opts += tokens[i];
		}
	}
	//
	// Sets or replaces a flag+value token in opts; empty value removes the flag entirely.
	// Rebuilds opts from tokens to guarantee clean formatting.
	//
	void DocumentBuildRasm::SetFlagValue(std::string& opts, const std::string& flag, const std::string& value) {
		std::vector<std::string> tokens;
		TokenizeOpts(opts, tokens);
		bool found = false;
		for (size_t i = 0; i < tokens.size(); i++) {
			if (tokens[i][0] != '-')
				continue;
			if (tokens[i] == flag) {
				found = true;
				if (value.empty()) {
					tokens.erase(tokens.begin() + (int)i);
					if (i < tokens.size() && tokens[i][0] != '-')
						tokens.erase(tokens.begin() + (int)i);
				} else {
					if (i + 1 < tokens.size() && tokens[i + 1][0] != '-')
						tokens[i + 1] = value;
					else
						tokens.insert(tokens.begin() + (int)i + 1, value);
				}
				break;
			}
		}
		if (!found && !value.empty()) {
			tokens.push_back(flag);
			tokens.push_back(value);
		}
		opts.clear();
		for (size_t i = 0; i < tokens.size(); i++) {
			if (i > 0)
				opts += ' ';
			opts += tokens[i];
		}
	}
	//
	// Renders a labelled path InputText tied to a flag+value token.
	// The buffer is only synced from opts when the widget is not actively focused
	// (external/initial load); changes typed by the user write back to opts.
	//
	void DocumentBuildRasm::RenderPathField(const char* label, const char* flag, char* buf, size_t bufSize, std::string& opts, const char* hint) {
		std::string current = GetFlagValue(opts, flag);
		//
		// Only sync the buffer from the stored value when the widget is not being actively
		// edited. Syncing every frame would overwrite the user's in-progress input with the
		// previous stored value, causing repeated appends in the command-line string.
		//
		std::string widgetId = std::string("##") + flag;
		if (!ImGui::IsItemActive() && current != buf)
			snprintf(buf, bufSize, "%s", current.c_str());
		float labelWidth = ImGui::CalcTextSize(label).x + ImGui::GetStyle().ItemSpacing.x;
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - labelWidth);
		bool changed = hint ? ImGui::InputTextWithHint(widgetId.c_str(), hint, buf, bufSize) : ImGui::InputText(widgetId.c_str(), buf, bufSize);
		if (changed) {
			SetFlagValue(opts, flag, buf[0] != '\0' ? std::string(buf) : "");
			RetrodevLib::Project::MarkAsModified();
			if (m_docModified)
				*m_docModified = true;
		}
		ImGui::SameLine();
		ImGui::TextDisabled("%s", label);
	}
	//
	// Build sub-panel: warnings/errors, compatibility, macro behaviour
	//
	void DocumentBuildRasm::RenderBuildOptions(RetrodevLib::SourceParams* params) {
		std::string& opts = params->toolOptions["RASM"];
		float fontSize = ImGui::GetFontSize();
		//
		// Warning and error control
		//
		ImGui::SeparatorText(ICON_ALERT_OUTLINE " Warnings and Errors");
		bool nowarning = HasFlag(opts, "-w");
		if (ImGui::Checkbox("Suppress all warnings (-w)", &nowarning)) {
			SetFlag(opts, "-w", nowarning);
			RetrodevLib::Project::MarkAsModified();
			if (m_docModified)
				*m_docModified = true;
		}
		bool nocrunchwarning = HasFlag(opts, "-wc");
		if (ImGui::Checkbox("Suppress slow-crunch warning (-wc)", &nocrunchwarning)) {
			SetFlag(opts, "-wc", nocrunchwarning);
			RetrodevLib::Project::MarkAsModified();
			if (m_docModified)
				*m_docModified = true;
		}
		bool erronwarn = HasFlag(opts, "-twe");
		if (ImGui::Checkbox("Treat warnings as errors (-twe)", &erronwarn)) {
			SetFlag(opts, "-twe", erronwarn);
			RetrodevLib::Project::MarkAsModified();
			if (m_docModified)
				*m_docModified = true;
		}
		bool extErr = HasFlag(opts, "-xr");
		if (ImGui::Checkbox("Extended error display (-xr)", &extErr)) {
			SetFlag(opts, "-xr", extErr);
			RetrodevLib::Project::MarkAsModified();
			if (m_docModified)
				*m_docModified = true;
		}
		bool warnUnused = HasFlag(opts, "-wu");
		if (ImGui::Checkbox("Warn on unused symbols (-wu)", &warnUnused)) {
			SetFlag(opts, "-wu", warnUnused);
			RetrodevLib::Project::MarkAsModified();
			if (m_docModified)
				*m_docModified = true;
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Emit a warning for every symbol that is defined but never referenced");
		//
		// Max errors: stored as "-me N"; 0 means flag absent (no limit)
		//
		std::string maxerrStr = GetFlagValue(opts, "-me");
		int maxerr = maxerrStr.empty() ? 0 : std::atoi(maxerrStr.c_str());
		ImGui::SetNextItemWidth(fontSize * 6.0f);
		if (ImGui::InputInt("Max errors, 0 = unlimited (-me)", &maxerr)) {
			if (maxerr < 0)
				maxerr = 0;
			SetFlagValue(opts, "-me", maxerr > 0 ? std::to_string(maxerr) : "");
			RetrodevLib::Project::MarkAsModified();
			if (m_docModified)
				*m_docModified = true;
		}
		//
		// Compatibility modes
		//
		ImGui::SeparatorText(ICON_TUNE " Compatibility");
		//
		// AS80 / UZ80 as a three-way combo
		//
		static const char* const k_as80Modes[] = {"None", "AS80 (-ass)", "UZ80 (-uz)"};
		int as80 = HasFlag(opts, "-ass") ? 1 : (HasFlag(opts, "-uz") ? 2 : 0);
		ImGui::SetNextItemWidth(fontSize * 14.0f);
		if (ImGui::BeginCombo("Assembler compatibility", k_as80Modes[as80])) {
			for (int i = 0; i < 3; i++) {
				bool sel = (as80 == i);
				if (ImGui::Selectable(k_as80Modes[i], sel)) {
					SetFlag(opts, "-ass", i == 1);
					SetFlag(opts, "-uz", i == 2);
					RetrodevLib::Project::MarkAsModified();
					if (m_docModified)
						*m_docModified = true;
				}
				if (sel)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		bool dams = HasFlag(opts, "-dams");
		if (ImGui::Checkbox("DAMS dot-label convention (-dams)", &dams)) {
			SetFlag(opts, "-dams", dams);
			RetrodevLib::Project::MarkAsModified();
			if (m_docModified)
				*m_docModified = true;
		}
		bool pasmo = HasFlag(opts, "-pasmo");
		if (ImGui::Checkbox("PASMO compatibility (-pasmo)", &pasmo)) {
			SetFlag(opts, "-pasmo", pasmo);
			RetrodevLib::Project::MarkAsModified();
			if (m_docModified)
				*m_docModified = true;
		}
		//
		// Maxam floor mode: -m present = floor truncation
		//
		bool maxamFloor = HasFlag(opts, "-m");
		if (ImGui::Checkbox("Maxam-style expression floor (-m)", &maxamFloor)) {
			SetFlag(opts, "-m", maxamFloor);
			RetrodevLib::Project::MarkAsModified();
			if (m_docModified)
				*m_docModified = true;
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Maxam-style floor truncation for floating-point expressions");
		bool noampersand = HasFlag(opts, "-amper");
		if (ImGui::Checkbox("Use & for hex values (-amper)", &noampersand)) {
			SetFlag(opts, "-amper", noampersand);
			RetrodevLib::Project::MarkAsModified();
			if (m_docModified)
				*m_docModified = true;
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Accept & as a hexadecimal prefix (e.g. &1A2B) in addition to the default 0x and # notations");
		bool freequote = HasFlag(opts, "-fq");
		if (ImGui::Checkbox("Free quote mode (-fq)", &freequote)) {
			SetFlag(opts, "-fq", freequote);
			RetrodevLib::Project::MarkAsModified();
			if (m_docModified)
				*m_docModified = true;
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Treat all characters inside quotes literally");
		bool utf8 = HasFlag(opts, "-utf8");
		if (ImGui::Checkbox("UTF-8 keyboard translation (-utf8)", &utf8)) {
			SetFlag(opts, "-utf8", utf8);
			RetrodevLib::Project::MarkAsModified();
			if (m_docModified)
				*m_docModified = true;
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Translate UTF-8 multi-byte keyboard sequences to the corresponding CPC character codes");
		//
		// Module separator:
		//
		std::string sepStr = GetFlagValue(opts, "-msep");
		char sepBuf[2] = {sepStr.empty() ? '.' : sepStr[0], '\0'};
		ImGui::SetNextItemWidth(fontSize * 3.0f);
		if (ImGui::InputText("Module separator (-msep)", sepBuf, sizeof(sepBuf))) {
			if (sepBuf[0] != '\0' && sepBuf[0] != '.') {
				SetFlagValue(opts, "-msep", std::string(1, sepBuf[0]));
				RetrodevLib::Project::MarkAsModified();
				if (m_docModified)
					*m_docModified = true;
			} else if (sepBuf[0] == '.') {
				SetFlagValue(opts, "-msep", "");
				RetrodevLib::Project::MarkAsModified();
				if (m_docModified)
					*m_docModified = true;
			}
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Character between module name and symbol name (default '.')");
		//
		// Macro behaviour
		//
		ImGui::SeparatorText(ICON_COGS " Macro Behaviour");
		bool macrovoid = HasFlag(opts, "-void");
		if (ImGui::Checkbox("Void macro mode (-void)", &macrovoid)) {
			SetFlag(opts, "-void", macrovoid);
			RetrodevLib::Project::MarkAsModified();
			if (m_docModified)
				*m_docModified = true;
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Accept macro calls without parameters even when parameters are declared");
		bool mml = HasFlag(opts, "-mml");
		if (ImGui::Checkbox("Multi-line macro parameters (-mml)", &mml)) {
			SetFlag(opts, "-mml", mml);
			RetrodevLib::Project::MarkAsModified();
			if (m_docModified)
				*m_docModified = true;
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Allow macro invocations to span multiple lines");
		//
		// Diagnostics and verbose output
		//
		ImGui::SeparatorText(ICON_CHART_BAR " Diagnostics");
		bool displayStats = HasFlag(opts, "-v");
		if (ImGui::Checkbox("Display assembler statistics (-v)", &displayStats)) {
			SetFlag(opts, "-v", displayStats);
			RetrodevLib::Project::MarkAsModified();
			if (m_docModified)
				*m_docModified = true;
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Print pass count, binary size and timings after a successful assembly");
		bool verboseMap = HasFlag(opts, "-map");
		if (ImGui::Checkbox("Verbose bank map output (-map)", &verboseMap)) {
			SetFlag(opts, "-map", verboseMap);
			RetrodevLib::Project::MarkAsModified();
			if (m_docModified)
				*m_docModified = true;
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Display detailed bank map and segment layout during early assembly stages");
		bool cprinfo = HasFlag(opts, "-cprquiet");
		if (ImGui::Checkbox("Suppress CPR bank info (-cprquiet)", &cprinfo)) {
			SetFlag(opts, "-cprquiet", cprinfo);
			RetrodevLib::Project::MarkAsModified();
			if (m_docModified)
				*m_docModified = true;
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Disable the detailed ROM/cartridge bank information printed in cartridge mode");
		//
		// Generated CLI string (read-only preview)
		//
		ImGui::Spacing();
		ImGui::SeparatorText(ICON_CONSOLE_LINE " Generated options");
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::InputText("##RasmOptsPreview", const_cast<char*>(opts.c_str()), opts.size() + 1, ImGuiInputTextFlags_ReadOnly);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Command-line flags that will be passed to RASM");
	}
	//
	// Output sub-panel: output radix and per-format file paths
	//
	void DocumentBuildRasm::RenderOutputOptions(RetrodevLib::SourceParams* params) {
		std::string& opts = params->toolOptions["RASM"];
		//
		// Output radix
		//
		ImGui::SeparatorText(ICON_EXPORT " Output Radix");
		bool autoRadix = HasFlag(opts, "-oa");
		if (ImGui::Checkbox("Automatic radix from input filename (-oa)", &autoRadix)) {
			SetFlag(opts, "-oa", autoRadix);
			RetrodevLib::Project::MarkAsModified();
			if (m_docModified)
				*m_docModified = true;
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Derive the output filename prefix from the input source filename");
		ImGui::BeginDisabled(autoRadix);
		RenderPathField("Output radix (-o)", "-o", m_outputRadixBuf, sizeof(m_outputRadixBuf), opts, "out/");
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Common prefix used for all generated output files");
		ImGui::EndDisabled();
		//
		// Binary output
		//
		ImGui::SeparatorText(ICON_FILE_EXPORT " Binary");
		RenderPathField("Binary file (-ob)", "-ob", m_binaryNameBuf, sizeof(m_binaryNameBuf), opts, "out/program.bin");
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Full output path for the raw binary file (.bin)");
		//
		// ROM / cartridge output
		//
		ImGui::SeparatorText(ICON_CHIP " ROM / Cartridge");
		RenderPathField("ROM file (-or)", "-or", m_romNameBuf, sizeof(m_romNameBuf), opts, "out/program.xpr");
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Full output path for the ROM (.xpr) file");
		RenderPathField("Cartridge file (-oc)", "-oc", m_cartridgeNameBuf, sizeof(m_cartridgeNameBuf), opts, "out/program.cpr");
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Full output path for the CPC cartridge (.cpr/.xpr) file");
		//
		// Snapshot output
		//
		ImGui::SeparatorText(ICON_MEMORY " Snapshot");
		RenderPathField("Snapshot file (-oi)", "-oi", m_snapshotNameBuf, sizeof(m_snapshotNameBuf), opts, "out/program.sna");
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Full output path for the Amstrad CPC snapshot (.sna) file");
		//
		// Tape output
		//
		ImGui::SeparatorText(ICON_CASSETTE " Tape");
		RenderPathField("Tape file (-ot)", "-ot", m_tapeNameBuf, sizeof(m_tapeNameBuf), opts, "out/program.cdt");
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Full output path for the tape image (.cdt/.tzx) file");
		//
		// Symbol and breakpoint files
		//
		ImGui::SeparatorText(ICON_LABEL_OUTLINE " Symbol / Breakpoint");
		RenderPathField("Symbol file (-os)", "-os", m_symbolNameBuf, sizeof(m_symbolNameBuf), opts, "out/program.sym");
		if (ImGui::IsItemHovered()) {
			bool osRasmAndSymConflict = HasFlag(opts, "-rasm") && (HasFlag(opts, "-s") || HasFlag(opts, "-sz") || HasFlag(opts, "-sw") || HasFlag(opts, "-sc"));
			if (osRasmAndSymConflict)
				ImGui::SetTooltip("-os cannot be used when both -rasm and a standard symbol format (-s/-sz/-sw/-sc) are active.\nBoth outputs share the same path slot.\nLeave -os empty and enable -oa so RASM auto-derives both .sym and .rasm filenames from the input filename.");
			else
				ImGui::SetTooltip("Full output path for the symbol / label export file.\nNote: if -rasm and a standard symbol format (-s/-sz/-sw/-sc) are both enabled, -os cannot be used;\nleave it empty and use -oa so both .sym and .rasm files are derived from the input filename automatically.");
		}
		RenderPathField("Breakpoint file (-ok)", "-ok", m_breakpointNameBuf, sizeof(m_breakpointNameBuf), opts, "out/program.brk");
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Full output path for the breakpoint export file");
		RenderPathField("CPR info file (-ol)", "-ol", m_cprinfoNameBuf, sizeof(m_cprinfoNameBuf), opts, "out/program.lst");
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Full output path for the ROM label (CPR info) file");
		//
		// Symbol export
		//
		ImGui::SeparatorText(ICON_IDENTIFIER " Symbol Export");
		//
		// Export format combo: None / Default / Pasmo / Winape / Custom
		// Maps to export_sym values 0-4; -s=1, -sz=2 (pasmo), -sp=3 (winape), -sc=4 (custom)
		//
		static const char* const k_symFormats[] = {"None", "Default (-s)", "Pasmo (-sz)", "Winape (-sw)", "Custom (-sc)"};
		static const char* const k_symFlags[] = {"", "-s", "-sz", "-sw", "-sc"};
		static const int k_symCount = 5;
		int symFormat = 0;
		for (int i = k_symCount - 1; i >= 1; i--) {
			if (HasFlag(opts, k_symFlags[i])) {
				symFormat = i;
				break;
			}
		}
		ImGui::SetNextItemWidth(ImGui::GetFontSize() * 16.0f);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Select the format used when writing the symbol / label export file (-os).\nNone: no symbol file is written.\nDefault (-s): plain address=label pairs.\nPasmo (-sz): Pasmo-compatible format.\nWinape (-sw): WinAPE breakpoint/symbol format.\nCustom (-sc): user-defined printf-style format string.");
		if (ImGui::BeginCombo("Symbol export format", k_symFormats[symFormat])) {
			for (int i = 0; i < k_symCount; i++) {
				bool sel = (symFormat == i);
				if (ImGui::Selectable(k_symFormats[i], sel)) {
					for (int j = 1; j < k_symCount; j++)
						SetFlag(opts, k_symFlags[j], false);
					if (i > 0)
						SetFlag(opts, k_symFlags[i], true);
					RetrodevLib::Project::MarkAsModified();
					if (m_docModified)
						*m_docModified = true;
				}
				if (sel)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		//
		// Custom format string: only shown when Custom is selected
		//
		if (symFormat == 4) {
			RenderPathField("Custom format (-sc)", "-sc", m_flexibleExportBuf, sizeof(m_flexibleExportBuf), opts);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Custom symbol export format string (e.g. \"%%s EQU %%d\")");
		}
		//
		// What to include in the export
		//
		bool exportLocal = HasFlag(opts, "-sl");
		if (ImGui::Checkbox("Include local labels (-sl)", &exportLocal)) {
			SetFlag(opts, "-sl", exportLocal);
			RetrodevLib::Project::MarkAsModified();
			if (m_docModified)
				*m_docModified = true;
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Include module-local labels (those prefixed with '@') in the exported symbol file");
		bool exportVar = HasFlag(opts, "-sv");
		if (ImGui::Checkbox("Include variable symbols (-sv)", &exportVar)) {
			SetFlag(opts, "-sv", exportVar);
			RetrodevLib::Project::MarkAsModified();
			if (m_docModified)
				*m_docModified = true;
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Include symbols defined with DEFW/DEFB/DEFD variable directives in the exported symbol file");
		bool exportEqu = HasFlag(opts, "-sq");
		if (ImGui::Checkbox("Include EQU symbols (-sq)", &exportEqu)) {
			SetFlag(opts, "-sq", exportEqu);
			RetrodevLib::Project::MarkAsModified();
			if (m_docModified)
				*m_docModified = true;
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Include EQU constant definitions in the exported symbol file");
		bool exportMulti = HasFlag(opts, "-sm");
		if (ImGui::Checkbox("Split symbols by memory bank (-sm)", &exportMulti)) {
			SetFlag(opts, "-sm", exportMulti);
			RetrodevLib::Project::MarkAsModified();
			if (m_docModified)
				*m_docModified = true;
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Write one symbol file per memory bank instead of a single combined file");
		bool enforceCase = HasFlag(opts, "-ec");
		if (ImGui::Checkbox("Preserve symbol case (-ec)", &enforceCase)) {
			SetFlag(opts, "-ec", enforceCase);
			RetrodevLib::Project::MarkAsModified();
			if (m_docModified)
				*m_docModified = true;
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Keep original label capitalisation instead of normalising to upper case");
		bool exportRasm = HasFlag(opts, "-rasm");
		if (ImGui::Checkbox("Export RASM super-symbol file (-rasm)", &exportRasm)) {
			SetFlag(opts, "-rasm", exportRasm);
			RetrodevLib::Project::MarkAsModified();
			if (m_docModified)
				*m_docModified = true;
		}
		if (ImGui::IsItemHovered()) {
			bool rasmAndSymConflict = exportRasm && (symFormat > 0);
			if (rasmAndSymConflict)
				ImGui::SetTooltip("Exports a RASM super-symbol file (.rasm) for use with ACE-DL.\nACE-DL requires symbols in .rasm format — the Symbol file path (-os) must use a .rasm extension.\nWARNING: -rasm and a standard symbol format (-s/-sz/-sw/-sc) are both active.\n-os cannot be shared between both outputs. Leave -os empty and enable -oa so RASM\nauto-derives both .sym and .rasm filenames from the input filename.");
			else
				ImGui::SetTooltip("Exports a RASM super-symbol file (.rasm) for use with ACE-DL.\nACE-DL requires symbols in .rasm format — the Symbol file path (-os) must use a .rasm extension.\nIf a standard symbol format (-s/-sz/-sw/-sc) is also enabled, do not set -os;\nleave it empty and use -oa so RASM auto-derives both .sym and .rasm files from the input filename.");
		}
		bool exportBrk = HasFlag(opts, "-eb");
		if (ImGui::Checkbox("Export breakpoints to file (-eb)", &exportBrk)) {
			SetFlag(opts, "-eb", exportBrk);
			RetrodevLib::Project::MarkAsModified();
			if (m_docModified)
				*m_docModified = true;
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Write breakpoints to the file specified by the breakpoint path above");
		//
		// DSK / EDSK options
		//
		ImGui::SeparatorText(ICON_FLOPPY_VARIANT " DSK / EDSK");
		bool edskOverwrite = HasFlag(opts, "-eo");
		if (ImGui::Checkbox("Overwrite existing DSK files (-eo)", &edskOverwrite)) {
			SetFlag(opts, "-eo", edskOverwrite);
			RetrodevLib::Project::MarkAsModified();
			if (m_docModified)
				*m_docModified = true;
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Overwrite a file on a DSK/EDSK disk image if a file with the same name already exists");
		//
		// Generated CLI string (read-only preview)
		//
		ImGui::Spacing();
		ImGui::SeparatorText(ICON_CONSOLE_LINE " Generated options");
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::InputText("##RasmOutPreview", const_cast<char*>(opts.c_str()), opts.size() + 1, ImGuiInputTextFlags_ReadOnly);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Command-line flags that will be passed to RASM");
	}
	//
	// Debug sub-panel: snapshot embed, breakpoint embed, CPR info export, unused-symbol warning
	//
	void DocumentBuildRasm::RenderDebugOptions(RetrodevLib::SourceParams* params) {
		std::string& opts = params->toolOptions["RASM"];
		//
		// Snapshot debug info
		//
		ImGui::SeparatorText(ICON_CAMERA_OUTLINE " Snapshot");
		bool exportSna = HasFlag(opts, "-ss");
		if (ImGui::Checkbox("Embed symbols in snapshot (-ss)", &exportSna)) {
			SetFlag(opts, "-ss", exportSna);
			RetrodevLib::Project::MarkAsModified();
			if (m_docModified)
				*m_docModified = true;
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Embed exported symbols inside the snapshot file (SYMB chunk, ACE-compatible)");
		bool exportSnabrk = HasFlag(opts, "-sb");
		if (ImGui::Checkbox("Embed breakpoints in snapshot (-sb)", &exportSnabrk)) {
			SetFlag(opts, "-sb", exportSnabrk);
			RetrodevLib::Project::MarkAsModified();
			if (m_docModified)
				*m_docModified = true;
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Embed breakpoints inside the snapshot file (BRKS/BRKC chunks)");
		bool snapshotV2 = HasFlag(opts, "-v2");
		if (ImGui::Checkbox("Write snapshot version 2 (-v2)", &snapshotV2)) {
			SetFlag(opts, "-v2", snapshotV2);
			RetrodevLib::Project::MarkAsModified();
			if (m_docModified)
				*m_docModified = true;
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Write a version 2 snapshot instead of the default version 3");
		//
		// CPR / ROM info
		//
		ImGui::SeparatorText(ICON_CHIP " CPR Info");
		bool cprinfoExport = HasFlag(opts, "-er");
		if (ImGui::Checkbox("Export ROM labels to CPR info file (-er)", &cprinfoExport)) {
			SetFlag(opts, "-er", cprinfoExport);
			RetrodevLib::Project::MarkAsModified();
			if (m_docModified)
				*m_docModified = true;
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Write ROM labels to the CPR info file (path set in the Output tab)");
		//
		// Generated CLI string (read-only preview)
		//
		ImGui::Spacing();
		ImGui::SeparatorText(ICON_CONSOLE_LINE " Generated options");
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::InputText("##RasmDbgPreview", const_cast<char*>(opts.c_str()), opts.size() + 1, ImGuiInputTextFlags_ReadOnly);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Command-line flags that will be passed to RASM");
	}
}
