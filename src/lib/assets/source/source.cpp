// --------------------------------------------------------------------------------------------------------------
//
//
//
//
// --------------------------------------------------------------------------------------------------------------

#include "source.h"
#include "source.rasm.h"
#include <project/project.h>

namespace RetrodevLib {

	//
	bool SourceBuild::Build(const SourceParams* params) {
		if (params == nullptr) {
			Log::Error(LogChannel::Build, "[Build] No build parameters provided.");
			return false;
		}
		if (params->sources.empty()) {
			Log::Warning(LogChannel::Build, "[Build] No source files in the build item.");
			return false;
		}
		//
		// Resolve project folder once — all relative paths are anchored here
		//
		const std::string projectDir = Project::GetProjectFolder();
		//
		// Expand path variables (e.g. $(sdk)) in sources and include dirs before dispatching to any tool
		//
		std::vector<std::string> expandedSources;
		expandedSources.reserve(params->sources.size());
		for (const auto& source : params->sources)
			expandedSources.push_back(Project::ExpandPath(source));
		std::vector<std::string> expandedIncludeDirs;
		expandedIncludeDirs.reserve(params->includeDirs.size());
		for (const auto& dir : params->includeDirs)
			expandedIncludeDirs.push_back(Project::ExpandPath(dir));
		//
		// Dispatch to the selected tool back-end
		//
		if (params->tool == "RASM") {
			const std::string& toolOpts = [&]() -> const std::string& {
				static const std::string empty;
				auto it = params->toolOptions.find("RASM");
				return (it != params->toolOptions.end()) ? it->second : empty;
			}();
			for (const auto& source : expandedSources) {
				if (!RasmImpl::Build(source, expandedIncludeDirs, params->defines, toolOpts, projectDir))
					return false;
			}
			return true;
		}
		//
		// Unknown tool
		//
		char msg[256];
		std::snprintf(msg, sizeof(msg), "[Build] Unknown build tool: '%s'", params->tool.c_str());
		Log::Error(LogChannel::Build, "%s", msg);
		return false;
	}

} // namespace RetrodevLib
