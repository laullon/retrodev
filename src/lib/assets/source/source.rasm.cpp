// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Source asset -- RASM assembler invocation.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------
//
// Note: Some of the parameters should be allocated because rasm tries to free them
//

#include "source.rasm.h"
#include <rasm.api.h>
#include <log/log.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <string>
#include <vector>
#include <filesystem>

namespace RetrodevLib {
	namespace RasmImpl {

		// -------------------------------------------------------------- //
		// Internal helpers                                               //
		// -------------------------------------------------------------- //

		//
		// Emit a formatted message on LogChannel::Build
		//
		static void Emit(LogLevel level, const char* format, ...) {
			char buf[1024];
			va_list args;
			va_start(args, format);
			std::vsnprintf(buf, sizeof(buf), format, args);
			va_end(args);
			if (level == LogLevel::Error)
				Log::Error(LogChannel::Build, "%s", buf);
			else if (level == LogLevel::Warning)
				Log::Warning(LogChannel::Build, "%s", buf);
			else
				Log::Info(LogChannel::Build, "%s", buf);
		}
		//
		// Returns true if flag is present as a whole word in opts
		//
		static bool HasFlag(const std::string& opts, const char* flag) {
			size_t flagLen = std::strlen(flag);
			size_t pos = opts.find(flag);
			if (pos == std::string::npos)
				return false;
			size_t end = pos + flagLen;
			bool startOk = (pos == 0 || opts[pos - 1] == ' ');
			bool endOk = (end == opts.size() || opts[end] == ' ');
			return startOk && endOk;
		}
		//
		// Returns the value following a flag token (e.g. "-me 5" -> "5"), empty if absent
		//
		static std::string GetFlagValue(const std::string& opts, const char* flag) {
			size_t flagLen = std::strlen(flag);
			size_t pos = opts.find(flag);
			if (pos == std::string::npos)
				return {};
			size_t valStart = pos + flagLen;
			if (valStart >= opts.size() || opts[valStart] != ' ')
				return {};
			valStart++;
			size_t valEnd = opts.find(' ', valStart);
			return opts.substr(valStart, valEnd == std::string::npos ? std::string::npos : valEnd - valStart);
		}
		//
		// Build a null-terminated C string vector owned by a std::vector<char*> for s_parameter path arrays.
		// Each pointer points into the corresponding std::string in the source vector.
		// The source strings must outlive the pointers.
		//
		static std::vector<char*> MakePtrArray(std::vector<std::string>& strings) {
			std::vector<char*> ptrs;
			ptrs.reserve(strings.size());
			for (auto& s : strings)
				ptrs.push_back(const_cast<char*>(s.c_str()));
			return ptrs;
		}

		//
		// Scan a RASM output line and replace every [absolute/path] or [absolute/path:line]
		// token with a path relative to projectDir, normalised to forward slashes.
		// Tokens that are not valid paths are left unchanged.
		//
		static std::string RelativiseRasmLine(const std::string& line, const std::string& projectDir) {
			if (projectDir.empty() || line.find('[') == std::string::npos)
				return line;
			std::string result;
			result.reserve(line.size());
			size_t i = 0;
			while (i < line.size()) {
				if (line[i] != '[') {
					result += line[i++];
					continue;
				}
				size_t close = line.find(']', i + 1);
				if (close == std::string::npos) {
					result += line[i++];
					continue;
				}
				std::string token = line.substr(i + 1, close - i - 1);
				//
				// Split optional trailing :line suffix before relativising the path part
				//
				std::string pathPart = token;
				std::string suffix;
				size_t colon = token.rfind(':');
				if (colon != std::string::npos && colon > 1) {
					bool allDigits = true;
					for (size_t k = colon + 1; k < token.size(); k++)
						if (token[k] < '0' || token[k] > '9') {
							allDigits = false;
							break;
						}
					if (allDigits) {
						pathPart = token.substr(0, colon);
						suffix = token.substr(colon);
					}
				}
				std::error_code ec;
				std::filesystem::path rel = std::filesystem::relative(std::filesystem::path(pathPart), std::filesystem::path(projectDir), ec);
				if (!ec && !rel.empty())
					result += '[' + rel.generic_string() + suffix + ']';
				else
					result += '[' + token + ']';
				i = close + 1;
			}
			return result;
		}

		bool Build(const std::string& source, const std::vector<std::string>& includeDirs, const std::vector<std::string>& defines, const std::string& toolOpts,
				   const std::string& projectDir) {
			//
			// Validate that source file is specified
			//
			if (source.empty()) {
				Emit(LogLevel::Warning, "[RASM] No source file specified.");
				return false;
			}
			std::string filename = source;
			//
			// Build s_parameter from the stored toolOpts CLI string plus explicit lists.
			// All pointer members default to NULL, all int members to 0.
			//
			struct s_parameter param;
			std::memset(&param, 0, sizeof(param));
			//
			// Apply the same defaults that rasm main() sets before CLI parsing.
			// maxerr=0 means unlimited: prevents MaxError from calling FreeAssenv+exit
			// inside PreProcessing before ae->wl is built (which crashes at offset 12).
			//
			param.cprinfo = 1;
			param.maxerr = 0;
			param.rough = 0.5f;
			param.module_separator = '_';
			//
			// Warning / error control
			//
			param.nowarning = HasFlag(toolOpts, "-w") ? 1 : 0;
			param.nocrunchwarning = HasFlag(toolOpts, "-wc") ? 1 : 0;
			param.erronwarn = HasFlag(toolOpts, "-twe") ? 1 : 0;
			param.extended_error = HasFlag(toolOpts, "-xr") ? 1 : 0;
			param.warn_unused = HasFlag(toolOpts, "-wu") ? 1 : 0;
			std::string maxerrStr = GetFlagValue(toolOpts, "-me");
			if (!maxerrStr.empty())
				param.maxerr = std::atoi(maxerrStr.c_str());
			//
			// Compatibility modes
			//
			param.as80 = HasFlag(toolOpts, "-ass") ? 1 : 0;
			int uz80 = HasFlag(toolOpts, "-uz") ? 1 : 0;
			//
			// rasm.h uses as80 for AS80 mode and a separate dams/pasmo for others;
			// UZ80 mode is not a named field -- it is passed via as80=2 internally if
			// the assembler supports it, but the public API only exposes as80/dams/pasmo.
			// Map -uz to dams=0, pasmo=0, as80=2 as best approximation.
			//
			if (uz80) {
				param.as80 = 2;
			}
			param.dams = HasFlag(toolOpts, "-dams") ? 1 : 0;
			param.pasmo = HasFlag(toolOpts, "-pasmo") ? 1 : 0;
			param.v2 = HasFlag(toolOpts, "-v2") ? 1 : 0;
			param.noampersand = HasFlag(toolOpts, "-amper") ? 1 : 0;
			param.freequote = HasFlag(toolOpts, "-fq") ? 1 : 0;
			param.utf8enable = HasFlag(toolOpts, "-utf8") ? 1 : 0;
			//
			// Module separator (single char; default '.' when absent)
			//
			std::string sepStr = GetFlagValue(toolOpts, "-msep");
			if (!sepStr.empty())
				param.module_separator = sepStr[0];
			//
			// Macro behaviour
			//
			param.macrovoid = HasFlag(toolOpts, "-void") ? 1 : 0;
			param.macro_multi_line = HasFlag(toolOpts, "-mml") ? 1 : 0;
			//
			// Diagnostics
			//
			param.display_stats = HasFlag(toolOpts, "-v") ? 1 : 0;
			param.verbose_assembling = HasFlag(toolOpts, "-map") ? 1 : 0;
			param.cprinfo = HasFlag(toolOpts, "-cprquiet") ? 0 : 1;
			//
			// Output options
			//
			param.automatic_radix = HasFlag(toolOpts, "-oa") ? 1 : 0;
			//
			// Output filenames -- values live in local std::strings so pointers stay valid.
			// Flag letters match the UI: -o=radix, -ob=binary, -or=ROM, -oc=cartridge,
			// -oi=snapshot, -ot=tape, -os=symbol, -ok=breakpoint, -ol=CPR-info, -sc=custom export format.
			//
			std::string outputRadix = GetFlagValue(toolOpts, "-o");
			std::string binaryName = GetFlagValue(toolOpts, "-ob");
			std::string romName = GetFlagValue(toolOpts, "-or");
			std::string cartridgeName = GetFlagValue(toolOpts, "-oc");
			std::string snapshotName = GetFlagValue(toolOpts, "-oi");
			std::string tapeName = GetFlagValue(toolOpts, "-ot");
			std::string symbolName = GetFlagValue(toolOpts, "-os");
			std::string breakpointName = GetFlagValue(toolOpts, "-ok");
			std::string cprinfoName = GetFlagValue(toolOpts, "-ol");
			std::string flexibleExport = GetFlagValue(toolOpts, "-sc");
			param.outputfilename = outputRadix.empty() ? nullptr : const_cast<char*>(outputRadix.c_str());
			param.binary_name = binaryName.empty() ? nullptr : const_cast<char*>(binaryName.c_str());
			param.rom_name = romName.empty() ? nullptr : const_cast<char*>(romName.c_str());
			param.cartridge_name = cartridgeName.empty() ? nullptr : const_cast<char*>(cartridgeName.c_str());
			param.snapshot_name = snapshotName.empty() ? nullptr : const_cast<char*>(snapshotName.c_str());
			param.tape_name = tapeName.empty() ? nullptr : const_cast<char*>(tapeName.c_str());
			param.symbol_name = symbolName.empty() ? nullptr : const_cast<char*>(symbolName.c_str());
			param.breakpoint_name = breakpointName.empty() ? nullptr : const_cast<char*>(breakpointName.c_str());
			param.cprinfo_name = cprinfoName.empty() ? nullptr : const_cast<char*>(cprinfoName.c_str());
			// Flexible export is expected from RASM to be own allocated so it can be freed
			//
			param.flexible_export = flexibleExport.empty() ? nullptr : strdup(const_cast<char*>(flexibleExport.c_str()));
			//
			// Symbol / debug export flags
			//
			param.export_local = HasFlag(toolOpts, "-sl") ? 1 : 0;
			param.export_var = HasFlag(toolOpts, "-sv") ? 1 : 0;
			param.export_equ = HasFlag(toolOpts, "-sq") ? 1 : 0;
			param.export_multisym = HasFlag(toolOpts, "-sm") ? 1 : 0;
			param.enforce_symbol_case = HasFlag(toolOpts, "-ec") ? 1 : 0;
			param.export_rasmSymbolFile = HasFlag(toolOpts, "-rasm") ? 1 : 0;
			param.export_brk = HasFlag(toolOpts, "-eb") ? 1 : 0;
			param.edskoverwrite = HasFlag(toolOpts, "-eo") ? 1 : 0;
			//
			// Symbol export format: -s=default, -sz=pasmo, -sw=winape, -sc=custom.
			// export_sym is the format selector; any non-zero value enables export.
			//
			param.export_sym = HasFlag(toolOpts, "-sc") ? 4 : HasFlag(toolOpts, "-sw") ? 3 : HasFlag(toolOpts, "-sz") ? 2 : HasFlag(toolOpts, "-s") ? 1 : 0;
			//
			// Snapshot debug embed flags
			//
			param.export_sna = HasFlag(toolOpts, "-ss") ? 1 : 0;
			param.export_snabrk = HasFlag(toolOpts, "-sb") ? 1 : 0;
			param.cprinfoexport = HasFlag(toolOpts, "-er") ? 1 : 0;
			param.export_tape = HasFlag(toolOpts, "-st") ? 1 : 0;
			//
			// Include directories: build a mutable char* array for param.pathdef.
			// rasm's MergePath treats each entry as a filename and calls rasm_GetPath on it,
			// which strips everything after the last separator. A trailing separator is required
			// so the directory itself is preserved (mirrors what rasm's CLI parser does).
			//
			std::vector<std::string> absIncludeDirs(includeDirs);
			for (auto& dir : absIncludeDirs) {
				if (!dir.empty() && dir.back() != '/' && dir.back() != '\\')
					dir += '/';
			}
			std::vector<char*> pathPtrs = MakePtrArray(absIncludeDirs);
			if (!pathPtrs.empty()) {
				param.pathdef = pathPtrs.data();
				param.npath = static_cast<int>(pathPtrs.size());
				param.mpath = 0;
			}
			//
			// Preprocessor defines: build a char* array for param.symboldef.
			// RASM expects "KEY=VALUE" or just "KEY" entries, but the processing loop
			// (rasm.c ~26272) skips any entry that has no '=' -- so bare defines must
			// get a default value of 1 appended.  The CLI path also uppercases all
			// define names (rasm.c ~32815); the CharWord table contains only A-Z, so
			// source identifiers are always tokenised as uppercase.  Mirror that here.
			//
			std::vector<std::string> defsCopy(defines);
			for (auto& d : defsCopy) {
				size_t eqPos = d.find('=');
				//
				// No '=': bare define -- RASM's loop would skip it entirely; append =1
				//
				if (eqPos == std::string::npos) {
					d += "=1";
					eqPos = d.size() - 2;
				}
				//
				// Uppercase the name part to match RASM's uppercase token convention
				//
				for (size_t i = 0; i < eqPos; i++)
					d[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(d[i])));
			}
			std::vector<char*> defPtrs = MakePtrArray(defsCopy);
			if (!defPtrs.empty()) {
				param.symboldef = defPtrs.data();
				param.nsymb = static_cast<int>(defPtrs.size());
				param.msymb = 0;
			}

			// If filename is used it should be duplicated (free in rasm)
			// if custom export symbols is used, for the string the same

			//
			// Assemble
			//
			Emit(LogLevel::Info, "[RASM] Starting assembly: %s", filename.c_str());
			// Rasm expect the filename to be own allocated to free it
			//
			param.filename = strdup(filename.c_str());

			char* textout = nullptr;
			int result = RasmAssembleIntegrated(&param, &textout);
			//
			// Parse assembler output and forward each line to the logger
			//
			if (textout != nullptr) {
				for (char* line = strtok(textout, "\n"); line != nullptr; line = strtok(nullptr, "\n")) {
					std::string relLine = RelativiseRasmLine(line, projectDir);
					if (std::strncmp(line, "Error:", 6) == 0)
						Emit(LogLevel::Error, "[RASM] %s", relLine.c_str());
					else if (std::strncmp(line, "Warning:", 8) == 0)
						Emit(LogLevel::Warning, "[RASM] %s", relLine.c_str());
					else
						Emit(LogLevel::Info, "[RASM] %s", relLine.c_str());
				}
				std::free(textout);
			}
			if (result != 0) {
				Emit(LogLevel::Error, "[RASM] Assembly failed.");
				return false;
			}
			return true;
		}

	}
}
